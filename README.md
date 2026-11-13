# NightCoop Hub on Windows

All Windows-related files are in `tools/hub/windows/`. The hub itself is developed by [Twenfys](https://github.com/Twenfys/nightcoop-hub).

## Build and run

1. Download the repository using **Code → Download ZIP**, then extract it.
2. Open `tools\hub\windows\` and double-click `start.bat`. Approve the UAC prompt.
3. The first run requires an internet connection, administrator privileges, and about 5 GB of free disk space. The script installs MSYS2 (MinGW-w64 GCC), downloads ENet, builds `NightCoopHub.exe`, and removes the build tools afterward. The process takes around 10–15 minutes. To keep the compiler installed, remove `-Clean` from `start.bat`.
4. On subsequent runs, the panel opens directly without rebuilding.

## Using the panel

- `1` — Opens the required UDP ports in Windows Firewall and starts the hub.
- `0` — Stops the hub and closes the ports.
- `2` — Detects your public IP and copies the `IP#fingerprint` address to the clipboard.
- `x` — Closes the panel. If the hub is running, it continues running in the background.

Give other players your `IP#fingerprint` address. They can connect from the game by going to **Co-op → Server → Add an own server**.

If your VPS provider has a separate firewall, you also need to allow UDP ports `47800` and `47810-47873` there.

## Files and settings

The `data` folder, located next to the executable, stores the key, logs, and extracted hub files.

Settings are stored in `NightCoopHub.ini`, which is created on the first run:

```ini
name=my-hub
region=
port=47800
shards=auto
max_clients=512
```

To move the hub to another machine without changing its key or fingerprint, copy the entire folder, including `data`. If you leave out `data`, the hub generates a new key.

## Licensing

- The hub repository does not publish a license. Do not distribute `NightCoopHub.exe`; it contains the compiled hub. Build it on the machine where you intend to run it.
- The files in `tools/hub/windows/` are licensed under GPL-3.0. See the `LICENSE` file in that folder. This license applies only to those files.

---

# هاب NightCoop روی ویندوز

تمام فایل‌های مربوط به ویندوز در پوشه‌ی `tools/hub/windows/` قرار دارند. توسعه‌دهنده‌ی خود هاب [Twenfys](https://github.com/Twenfys/nightcoop-hub) است.

## ساخت و اجرا

1. مخزن را از طریق **Code → Download ZIP** دانلود و سپس از حالت فشرده خارج کن.
2. وارد پوشه‌ی `tools\hub\windows\` شو و روی `start.bat` دوبار کلیک کن. درخواست UAC را تأیید کن.
3. اجرای اول به اینترنت، دسترسی Administrator و حدود ۵ گیگابایت فضای خالی نیاز دارد. اسکریپت MSYS2 (شامل کامپایلر MinGW-w64 GCC) را نصب می‌کند، ENet را دانلود می‌کند و فایل `NightCoopHub.exe` را می‌سازد. بعد از پایان ساخت، ابزارهای نصب‌شده را پاک می‌کند. این فرایند حدود ۱۰ تا ۱۵ دقیقه طول می‌کشد. اگر می‌خواهی کامپایلر روی سیستم بماند، گزینه‌ی `-Clean` را از `start.bat` حذف کن.
4. از اجرای دوم به بعد، پنل مستقیماً باز می‌شود و نیازی به ساخت دوباره نیست.

## کار با پنل

- `1`: پورت‌های UDP موردنیاز را در فایروال ویندوز باز می‌کند و هاب را اجرا می‌کند.
- `0`: هاب را متوقف می‌کند و پورت‌ها را می‌بندد.
- `2`: IP عمومی را پیدا می‌کند و آدرس `IP#fingerprint` را در کلیپ‌بورد کپی می‌کند.
- `x`: پنل را می‌بندد. اگر هاب در حال اجرا باشد، به کارش ادامه می‌دهد.

آدرس `IP#fingerprint` را برای بازیکن‌های دیگر بفرست. برای اتصال در بازی، از مسیر **Co-op → Server → Add an own server** استفاده کنند.

اگر VPS فایروال جداگانه‌ای دارد، پورت‌های UDP `47800` و `47810-47873` را در تنظیمات آن هم باز کن.

## فایل‌ها و تنظیمات

پوشه‌ی `data` در کنار فایل اجرایی قرار دارد و کلید، لاگ‌ها و فایل‌های استخراج‌شده‌ی هاب را نگه می‌دارد.

تنظیمات در فایل `NightCoopHub.ini` ذخیره می‌شوند. این فایل در اولین اجرا ساخته می‌شود:

```ini
name=my-hub
region=
port=47800
shards=auto
max_clients=512
```

برای انتقال هاب به سیستم دیگر و حفظ همان کلید و fingerprint، کل پوشه را همراه با `data` کپی کن. اگر این پوشه را منتقل نکنی، هاب یک کلید جدید می‌سازد.

## مجوز انتشار

- مخزن خود هاب لایسنس مشخصی منتشر نکرده است. فایل `NightCoopHub.exe` را توزیع نکن، چون نسخه‌ی کامپایل‌شده‌ی هاب را در خود دارد. ساخت را روی همان سیستمی انجام بده که قرار است هاب روی آن اجرا شود.
- فایل‌های داخل `tools/hub/windows/` تحت مجوز GPL-3.0 منتشر شده‌اند. متن مجوز در فایل `LICENSE` همان پوشه قرار دارد. این مجوز فقط شامل همین فایل‌هاست.
