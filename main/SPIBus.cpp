#include <SPIBus.h>

void configurePinAsOutput(gpio_num_t pin){
  ESP_LOGI(__FUNCTION__, "pin=%d", pin);
	gpio_pad_select_gpio(pin);
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

void configureChipSelectPin(gpio_num_t csPin){
	configurePinAsOutput(csPin);
	gpio_set_level(csPin, 1);// Release slave
}

SPIBus::SPIBus(spi_host_device_t host): m_host(host){
}

SPIBus::~SPIBus(){
	  ESP_LOGI(__FUNCTION__, "... Freeing bus.");
	  ESP_ERROR_CHECK(::spi_bus_free(m_host));
}

esp_err_t SPIBus::init(gpio_num_t mosiPin, gpio_num_t misoPin, gpio_num_t clkPin){
	ESP_LOGD(__FUNCTION__, "init: mosi=%d, miso=%d, clk=%d", mosiPin, misoPin, clkPin);

	configurePinAsOutput(clkPin);
	configurePinAsOutput(mosiPin);
	gpio_set_direction(misoPin, GPIO_MODE_INPUT);

	spi_bus_config_t bus_config;
	bus_config.sclk_io_num     = clkPin;  // CLK
	bus_config.mosi_io_num     = mosiPin; // MOSI
	bus_config.miso_io_num     = misoPin; // MISO
	bus_config.quadwp_io_num   = -1;      // Not used
	bus_config.quadhd_io_num   = -1;      // Not used
	bus_config.max_transfer_sz = 4096;       // 0 means use default.
    bus_config.flags           = (SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MISO);
    bus_config.intr_flags =0;

	ESP_LOGI(__FUNCTION__, "... Initializing bus; host=%d", m_host);

	esp_err_t errRc = ::spi_bus_initialize(
			m_host,
			&bus_config,
			1 // DMA Channel
	);

	if (errRc != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "spi_bus_initialize(): rc=%d", errRc);
		abort();
	}
	return ESP_OK;
}

SPIDevice::SPIDevice(SPIBus* spiBus, gpio_num_t csPin): spiBus(spiBus), csPin(csPin), m_handle(nullptr) {
	configureChipSelectPin(csPin);
}

SPIDevice::~SPIDevice(){
	  ESP_LOGI(__FUNCTION__, "... Removing device.");
	  ESP_ERROR_CHECK(::spi_bus_remove_device(m_handle));
}

esp_err_t SPIDevice::init(){
	ESP_LOGD(__FUNCTION__, "init: cs=%d", csPin);
	spi_device_interface_config_t dev_config;
	dev_config.address_bits     = 0;
	dev_config.command_bits     = 0;
	dev_config.dummy_bits       = 0;
	dev_config.mode             = 0;
	dev_config.duty_cycle_pos   = 0;
	dev_config.cs_ena_posttrans = 0;
	dev_config.cs_ena_pretrans  = 0;
	dev_config.clock_speed_hz   = 100000;
	dev_config.spics_io_num     = csPin;
	dev_config.flags            = SPI_DEVICE_NO_DUMMY;
	dev_config.queue_size       = 1;
	dev_config.pre_cb           = NULL;
	dev_config.post_cb          = NULL;
	ESP_LOGI(__FUNCTION__, "... Adding device bus.");
	esp_err_t errRc = ::spi_bus_add_device(spiBus->m_host, &dev_config, &m_handle);
	if (errRc != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "spi_bus_add_device(): rc=%d", errRc);
		abort();
	}
	return errRc;
}

void SPIDevice::transfer(uint8_t* data, size_t dataLen){
	assert(data != nullptr);
	assert(dataLen > 0);
#ifdef DEBUG
	ESP_LOG_BUFFER_HEXDUMP("  spiData ", data, dataLen, ESP_LOG_DEBUG);
	for (auto i = 0; i < dataLen; i++) {
		ESP_LOGD(__FUNCTION__, "> %2d %.2x", i, data[i]);
	}
#endif
	spi_transaction_t trans_desc;
	trans_desc.addr   = 0;
	trans_desc.cmd   = 0;
	trans_desc.flags     = 0;
	trans_desc.length    = dataLen * 8;
	trans_desc.rxlength  = 0;
	trans_desc.tx_buffer = data;
	trans_desc.rx_buffer = data;
	trans_desc.user=0;

	esp_err_t rc = ::spi_device_transmit(m_handle, &trans_desc);
	if (rc != ESP_OK) {
		ESP_LOGE(__FUNCTION__, "transfer:spi_device_transmit: %d", rc);
	}
}
