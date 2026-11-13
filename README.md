# NightCoop Hub on Windows

Everything for Windows is in `tools/hub/windows/`. The hub itself is by [Twenfys](https://github.com/Twenfys/nightcoop-hub).

## Build and run

1. Download the repository (Code, Download ZIP) and unpack it.
2. Open `tools\hub\windows` and double-click `start.bat`. Accept the UAC prompt.
3. The first run needs internet, administrator rights and about 5 GB of free disk space. It installs MSYS2 (MinGW-w64 GCC), downloads ENet, builds `NightCoopHub.exe`, then removes what it installed for the build. About 10-15 minutes. Remove `-Clean` from `start.bat` to keep the compiler.
4. Next runs open the panel directly.

## Panel

- `1` opens the UDP ports in Windows Firewall and starts the hub
- `0` stops the hub and closes the ports
- `2` finds the public IP and copies `IP#fingerprint` to the clipboard
- `x` closes the window, a started hub keeps running

Give players the `IP#fingerprint` address. In the game: Co-op, Server, Add an own server.

If your VPS provider has its own firewall, open UDP `47800` and `47810-47873` there too.

## Files

The key, logs and the unpacked hub are in the `data` folder next to the exe. Settings are in `NightCoopHub.ini`, created on the first run:

```
name=my-hub
region=
port=47800
shards=auto
max_clients=512
```

Copy the whole folder with `data` to another machine to keep the same key and fingerprint. Without `data` a new key is created.

## Notes

- The hub repository does not publish a license. Do not share `NightCoopHub.exe`, it contains the compiled hub. Build it on the machine that will run it.
- The files in `tools/hub/windows` are under GPL-3.0 (see the LICENSE file in that folder). It covers only them.

---

# هاب NightCoop روی ویندوز

همه‌چیز مربوط به ویندوز در `tools/hub/windows/` است. خود هاب را [Twenfys](https://github.com/Twenfys/nightcoop-hub) نوشته.

## ساخت و اجرا

1. مخزن را دانلود کن (Code ← Download ZIP) و باز کن.
2. پوشه‌ی `tools\hub\windows` را باز کن و `start.bat` را دابل‌کلیک کن. UAC را تأیید کن.
3. اجرای اول به اینترنت، دسترسی Administrator و حدود ۵ گیگابایت فضای خالی نیاز دارد. MSYS2 (کامپایلر) را نصب می‌کند، ENet را می‌گیرد، `NightCoopHub.exe` را می‌سازد و بعد چیزهایی که برای ساخت نصب کرده را پاک می‌کند. حدود ۱۰ تا ۱۵ دقیقه. برای اینکه کامپایلر بماند، `-Clean` را از `start.bat` بردار.
4. دفعه‌های بعد پنل مستقیم باز می‌شود.

## پنل

- `1`: پورت‌های UDP را در فایروال ویندوز باز می‌کند و هاب را روشن می‌کند
- `0`: هاب را خاموش می‌کند و پورت‌ها را می‌بندد
- `2`: IP عمومی را پیدا می‌کند و `IP#fingerprint` را کپی می‌کند
- `x`: پنجره بسته می‌شود، هابِ روشن‌شده به کارش ادامه می‌دهد

آدرس `IP#fingerprint` را به بازیکن‌ها بده. در بازی: Co-op ← Server ← Add an own server.

اگر ارائه‌دهنده‌ی VPS فایروال جدا دارد، پورت‌های UDP شماره‌ی `47800` و `47810-47873` را آنجا هم باز کن.

## فایل‌ها

کلید، لاگ‌ها و هابِ بازشده در پوشه‌ی `data` کنار exe هستند. تنظیمات در `NightCoopHub.ini` است که اجرای اول ساخته می‌شود. اگر کل پوشه را با `data` به سیستم دیگری ببری، کلید و fingerprint همان می‌ماند. بدون `data` کلید جدید ساخته می‌شود.

## نکته‌ها

- مخزن هاب لایسنس منتشر نکرده. `NightCoopHub.exe` را پخش نکن، چون هاب کامپایل‌شده داخلش است. روی همان سیستمی که اجرا می‌شود بسازش.
- فایل‌های `tools/hub/windows` با GPL-3.0 منتشر شده‌اند (فایل LICENSE همان پوشه). این لایسنس فقط شامل همان‌هاست.
