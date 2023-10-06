# makerbadge_kuba1
Jednoduchá aplikace pro Maker Badge (https://www.makermarket.cz/maker-badge/) revize D.

Střídá po 180 sec (nebo dotyku na prostřední plošku) tři bitmapy (page1 až page3 v bitmap.h). 
Čtvrtou (page0) zobrazuje po stisku BOOT - ta je myšlená jako úvodní k vypnutému zařízení. 

Pokud střídá bitmapy po uplynutí času (ne dotykem), tak před tím zabliká ledkami, aby na sebe upozornila.

Měří a do levého dolního rohu displeje vypisuje napětí baterky a procentuální stav. 
Při příliš malém napětí vypíše na displej upozornění a uspí se na dlouho, aby nedošlo ke zničení baterky (ale ta má stejně ochranný obvod, takže by to mělo být OK v každém případě).

Bitmapy jsou v adresáři imgs/ a konverze do zdrojáku se provádí nástrojem https://javl.github.io/image2cpp/