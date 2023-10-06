# makerbadge_kuba1
Jednoduchá aplikace pro Maker Badge (https://www.makermarket.cz/maker-badge/) revize D.

Střídá po 180 sec (nebo dotyku na prostřední plošku) pět bitmap (page1 až page5 v bitmap.h). 
Šestou (page0) zobrazuje po stisku BOOT - ta je myšlená jako úvodní k vypnutému zařízení. 

Pokud střídá bitmapy po uplynutí času (ne dotykem), tak před tím zabliká ledkami, aby na sebe upozornila.

Měří a do levého dolního rohu displeje vypisuje napětí baterky a procentuální stav. 
Při příliš malém napětí vypíše na displej upozornění a uspí se na dlouho, aby nedošlo ke zničení baterky (ale ta má stejně ochranný obvod, takže by to mělo být OK v každém případě).

Bitmapy jsou v adresáři imgs/ a konverze do zdrojáku se provádí nástrojem https://javl.github.io/image2cpp/
Proti reálnému zobrazení na displeji je třeba je kreslit inverzně; ukládat jako 1-bit PNG a otočené 90° doprava.
Rozlišení bitmap je stejné jako rozlišení displeje, 128x250.

Pokud by bylo požadované vypisovat text (ne jen obrázek) s diakritikou, postupuj podle návodu zde: https://github.com/petrbrouzda/fontconvert8-iso8859-2

---

Kompilováno s Arduino Core pro ESP32 verze 2.0.11

Požadované knihovny:
* Adafruit NeoPixel at version 1.11.0
* GxEPD2 at version 1.5.2 
* Adafruit GFX Library at version 1.11.8
* Adafruit BusIO at version 1.14.4
* Wire at version 2.0.0 
* SPI at version 2.0.0 
* ESP32AnalogRead at version 0.2.1

Nastavení desky: 
* esp32s2 dev module
* CPU 80 mhz
* flash 40 mhz
* usb-cdc on boot: enabled

tedy FQBN: esp32:esp32:esp32s2:CDCOnBoot=cdc,CPUFreq=80,FlashFreq=40

Pro programování desky je třeba jí přepnout do programovacího režimu: stisknout a držet BOOT (vlevo), kliknout na RESET (vpravo) a pak teprve pustit BOOT. 
Po skončení programování vypíše Arduino IDE chybu, že nemůže desku v tomto režimu samo zresetovat, takže je potřeba ručně zmáčknout RESET.

Pokud chceš používat debug výpisy na sériový port, odkomentuj

`// #define LOGUJ_DEBUG`

a tím zajistíš, že to při startu bude 2 sekundy čekat, a sériový port se stihne napojit. Pro normální provoz lépe zakomentovat, aby to nemělo pomalé reakce.

Na Windows nemá "staré" Arduino 1.8.x sériový port ESP32-S2 rádo; doporučuju používat Arduino IDE 2.0.x.

