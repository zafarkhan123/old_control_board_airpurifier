/**
 * @file ethernetDriver.c
 *
 * @brief ethernet Driver
 *
 * @dir ethernetDriver
 * @brief 
 *
 * @author matfio
 * @date 2021.11.17
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "setting.h"
#include "config.h"

#include "esp_log.h"
#include "esp_eth.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "ethernetDriver/ethernetDriver.h"
#include "mcuDriver/mcuDriver.h"

#include "enc28j60/enc28j60.h"

#include "esp_event.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char *TAG = "ethD";

static bool sCommunicationWithEnc28j60Detected;

static spi_device_handle_t sSpiHandle;
static esp_netif_t *sEthNetif;
static eth_event_t sEthEvent;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Initialize spi
 *  @return return true if success
 */
static bool spiInit(void);

/** @brief DeInitialize spi
 */
static void spiDeInit(void);

/** @brief Get MAC ethernet address 
 *  @return return mac
 */
static mcuDriverMacAddress_t getEthernetMacAddress(void);

/** @brief Event handler for Ethernet events 
 */
static void ethEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/** @brief Event handler for IP_EVENT_ETH_GOT_IP
 */
static void gotIpEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool ethernetDriverInit(void)
{
    bool res = true;

    res &= spiInit();

    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    sEthNetif = esp_netif_new(&netif_cfg);

    // Set default handlers to process TCP/IP stuffs
    ESP_ERROR_CHECK(esp_eth_set_default_handlers(sEthNetif));

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &ethEventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &gotIpEventHandler, NULL));

    eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(sSpiHandle);
    enc28j60_config.int_gpio_num = CFG_ETHERNET_INT_GPIO;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = -1;  // ENC28J60 doesn't have SMI interface
    mac_config.smi_mdio_gpio_num = -1;
    esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
    phy_config.reset_gpio_num = -1; // ENC28J60 doesn't have a pin to reset internal PHY
    esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    esp_err_t status = esp_eth_driver_install(&eth_config, &eth_handle);

    ESP_LOGI(TAG, "esp eth driver install status %d", status);

    if(status != ESP_OK){
        ESP_LOGI(TAG, "connection with enc28j60 not detected");
        ESP_LOGW(TAG, "device without ethernet extension");

        spiDeInit();

        ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, &ethEventHandler));
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, &gotIpEventHandler));

        sEthEvent = ETHERNET_EVENT_STOP;
        
        return true;
    }

    ESP_LOGI(TAG, "device with ethernet extension");
    sCommunicationWithEnc28j60Detected = true;

    // ENC28J60 doesn't burn any factory MAC address, we need to set it manually.
    mcuDriverMacAddress_t espEthMac = getEthernetMacAddress();
    mac->set_addr(mac, espEthMac.part);
    
    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(sEthNetif, esp_eth_new_netif_glue(eth_handle)));
    
    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    return res;
}

bool ethernetDriverIsAdditionalPcbConnected(void)
{
    return sCommunicationWithEnc28j60Detected;
}

eth_event_t ethernetDriverGetStatus(void)
{
    return sEthEvent;
}

/*****************************************************************************
                         PRIVATE FUNCTION IMPLEMENTATION
*****************************************************************************/

static mcuDriverMacAddress_t getEthernetMacAddress(void)
{
    mcuDriverMacAddress_t wifiMacAddress = {};
   
    esp_read_mac(wifiMacAddress.part, ESP_MAC_ETH);

    ESP_LOGI(TAG, "Esp ethernet mac addr %02x:%02x:%02x:%02x:%02x:%02x",
                 wifiMacAddress.part[0], wifiMacAddress.part[1], wifiMacAddress.part[2], wifiMacAddress.part[3], wifiMacAddress.part[4], wifiMacAddress.part[5]);

    return wifiMacAddress;
}

static bool spiInit(void)
{
    /* ENC28J60 ethernet driver is based on spi driver */
    spi_device_interface_config_t devcfg = {
        .command_bits = 3,
        .address_bits = 5,
        .mode = 0,
        .clock_speed_hz = ((CFG_SPI_CLOCK_MHZ) * (1000U) * (1000U)),
        .spics_io_num = CFG_ETHERNET_CS_GPIO,
        .queue_size = 20
    };

    if(spi_bus_add_device(CFG_SPI_HOST_NUMBER, &devcfg, &sSpiHandle) != ESP_OK){
        return false;
    }
    
    return true;
}

static void spiDeInit(void)
{
    spi_bus_remove_device(sSpiHandle);

    gpio_reset_pin(CFG_ETHERNET_CS_GPIO);
    gpio_reset_pin(CFG_ETHERNET_INT_GPIO);
}

static void ethEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    sEthEvent = event_id;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

static void gotIpEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
}