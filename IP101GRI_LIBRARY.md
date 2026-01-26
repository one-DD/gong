# IP101GRI Ethernet PHY library (ESP32-P4 Nano)

The ESP32-P4 Nano board uses an IP101GRI 10/100 Ethernet PHY. The driver
is already part of ESP-IDF's `esp_eth` component, so you do not need a
separate third-party library. This note shows the minimal pieces to enable it.

## ESP-IDF (recommended)
1) Enable the PHY in menuconfig:
   Component config -> Ethernet -> PHY -> IP101 (CONFIG_ETH_PHY_IP101=y)

2) Include the driver header and create the PHY:
   #include "esp_eth.h"
   #include "esp_eth_mac.h"
   #include "esp_eth_phy_ip101.h"

   esp_eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
   esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);

3) Create the MAC for your SoC and wire pins:
   - Use the SoC-specific MAC factory from ESP-IDF ethernet examples.
   - Fill in RMII pins (MDC, MDIO, TXD0/1, RXD0/1, CRS_DV, TXEN, REF_CLK)
     from the board schematic.

4) Install and start the driver (see the esp-idf ethernet examples):
   - esp_eth_driver_install(...)
   - esp_eth_start(...)

## Arduino-ESP32
Arduino's built-in ETH library uses ESP-IDF under the hood. If your core
version exposes IP101 as a PHY type (e.g. ETH_PHY_IP101), you can call
ETH.begin(...) with the board pin mapping. If the PHY type is not exposed,
use ESP-IDF directly.

## References
- ESP-IDF Ethernet examples (examples/ethernet)
