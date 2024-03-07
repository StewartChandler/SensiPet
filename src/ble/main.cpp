#include "ble/BLE.h"
#include "mbed-trace/mbed_trace.h"
#include <events/mbed_events.h>
#include <pretty_printer.h>

/** This example demonstrates advertising and scanning for Bluetooth 4.1 devices
 */

using namespace std::literals::chrono_literals;

events::EventQueue event_queue;

static const char DEVICE_NAME[] = "Periodic";

/** Demonstrate advertising and scanning for Bluetooth 4.1 devices
 */
class BluetoothDemo : private mbed::NonCopyable<BluetoothDemo>,
                      public ble::Gap::EventHandler {
  public:
    BluetoothDemo(BLE& ble, events::EventQueue& event_queue)
        : _ble(ble), _event_queue(event_queue) {}

    ~BluetoothDemo() {
        if (_ble.hasInitialized()) {
            _ble.shutdown();
        }
    }

    /** Start BLE interface initialisation */
    void run() {
        /* handle gap events */
        _ble.gap().setEventHandler(this);

        ble_error_t error = _ble.init(this, &BluetoothDemo::on_init_complete);
        if (error) {
            print_error(error, "Error returned by BLE::init\r\n");
            return;
        }

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    }

  private:
    /** This is called when BLE interface is initialised and starts the first mode */
    void on_init_complete(BLE::InitializationCompleteCallbackContext* event) {
        if (event->error) {
            print_error(event->error, "Error during the initialisation\r\n");
            return;
        }

        print_mac_address();

        /* all calls are serialised on the user thread through the event queue */
        start_role();
    }

    void start_role() {
        /* This example is designed to be run on two boards at the same time,
         * depending on our role we will either be the advertiser or scanner,
         * until the roles are established we will cycle the roles until we find each
         * other */
        if (_role_established) {
            if (_is_scanner) {
                _event_queue.call(this, &BluetoothDemo::scan);
            }
            else {
                _event_queue.call(this, &BluetoothDemo::advertise);
            }
        }
        else {
            _is_scanner = !_is_scanner;

            if (_is_scanner) {
                _event_queue.call(this, &BluetoothDemo::scan);
            }
            else {
                _event_queue.call(this, &BluetoothDemo::advertise);
            }
        }
    }

    /** Set up and start advertising */
    void advertise() {
        ble_error_t error;

        /* Set advertising parameters */
        static const ble::AdvertisingParameters advertising_params(
            ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(25)),
            ble::adv_interval_t(ble::millisecond_t(50))
        );

        error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE, advertising_params
        );

        if (error) {
            print_error(error, "Gap::setAdvertisingParameters() failed\r\n");
            return;
        }

        ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE> data_builder;
        data_builder.setFlags();
        data_builder.setName(DEVICE_NAME);

        /* Set payload for advertising */
        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE, data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "Gap::setAdvertisingPayload() failed\r\n");
            return;
        }

        /* since we have two boards which might start running this example at the same
         * time we randomise the interval of advertising to have them meet when one is
         * advertising and the other one is scanning
         */
        ble::millisecond_t random_duration_ms((2 + rand() % 5) * 1000);
        ble::adv_duration_t random_duration(random_duration_ms);

        /* Start advertising */
        error =
            _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE, random_duration);

        if (error) {
            print_error(error, "Gap::startAdvertising() failed\r\n");
            return;
        }

        printf("Advertising started for %dms\r\n", random_duration_ms);
    }

    /** Set up and start scanning */
    void scan() {
        _is_connecting = false;

        ble::ScanParameters scan_params;
        scan_params.setOwnAddressType(ble::own_address_type_t::RANDOM);

        ble_error_t error = _ble.gap().setScanParameters(scan_params);

        if (error) {
            print_error(error, "Error caused by Gap::setScanParameters\r\n");
            return;
        }

        error = _ble.gap().startScan(ble::scan_duration_t(500));

        if (error) {
            print_error(error, "Error caused by Gap::startScan\r\n");
            return;
        }

        printf("Scanning started\r\n");
    }

  private:
    /* Gap::EventHandler */

    void onAdvertisingStart(const ble::AdvertisingStartEvent& event) override {
        /* No periodic advertising in Bluetooth 4.1 */
        printf("onAdvertising start. \r\n");
    }

    /** Look at scan payload to find a peer device and connect to it */
    void onAdvertisingReport(const ble::AdvertisingReportEvent& event) override {
        /* don't bother with analysing scan result if we're already connecting */
        if (_is_connecting) {
            return;
        }

        ble::AdvertisingDataParser adv_parser(event.getPayload());

        /* parse the advertising payload, looking for a discoverable device */
        while (adv_parser.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_parser.next();

            /* identify peer by name */
            if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME &&
                field.value.size() == strlen(DEVICE_NAME) &&
                (memcmp(field.value.data(), DEVICE_NAME, field.value.size()) == 0))
            {
                printf("We found the peer, connecting\r\n");

                ble_error_t error = _ble.gap().connect(
                    event.getPeerAddressType(),
                    event.getPeerAddress(),
                    ble::ConnectionParameters() // use the default connection parameters
                );

                if (error) {
                    print_error(error, "Error caused by Gap::connect\r\n");
                    return;
                }

                /* we may have already scan events waiting to be processed
                 * so we need to remember that we are already connecting
                 * and ignore them */
                _is_connecting = true;

                return;
            }
        }
    }

    void onAdvertisingEnd(const ble::AdvertisingEndEvent& event) override {
        printf("Advertising ended.\r\n");
        if (!event.isConnected()) {
            printf("No device connected to us, switch modes.\r\n");
            start_role();
        }
    }

    void onScanTimeout(const ble::ScanTimeoutEvent&) override {
        printf("Scanning ended\r\n");
        if (!_is_connecting) {
            printf("Failed to find peer\r\n");
            start_role();
        }
    }

    /** This is called by Gap to notify the application we connected */
    void onConnectionComplete(const ble::ConnectionCompleteEvent& event) override {
        if (event.getStatus() == BLE_ERROR_NONE) {
            printf("Connected to: ");
            print_address(event.getPeerAddress().data());
            printf("Roles established\r\n");
            _role_established = true;

            if (_is_scanner) {
                printf("I will disconnect\r\n");
                _event_queue.call_in(
                    1000ms,
                    [this, handle = event.getConnectionHandle()] {
                        _ble.gap().disconnect(
                            handle,
                            ble::local_disconnection_reason_t(
                                ble::local_disconnection_reason_t::USER_TERMINATION
                            )
                        );
                    }
                );
            }
            else {
                printf("I will keep advertising\r\n");
            }
        }
        else {
            printf("Failed to connect\r\n");
            start_role();
        }
    }

    /** This is called by Gap to notify the application we disconnected */
    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent& event) override {
        printf("Disconnected\r\n");
    }

  private:
    BLE& _ble;
    events::EventQueue& _event_queue;

    bool _is_scanner = false;
    bool _is_connecting = false;
    bool _role_established = false;
};

/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext* context) {
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main() {
    mbed_trace_init();

    BLE& ble = BLE::Instance();

    /* this will inform us off all events so we can schedule their handling
     * using our event queue */
    ble.onEventsToProcess(schedule_ble_events);

    /* look for other device and then settle on a role */
    BluetoothDemo demo(ble, event_queue);

    demo.run();

    return 0;
}
