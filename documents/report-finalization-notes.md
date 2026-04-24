# Report Finalization Notes

## 1. Chay du an: `pio run` hay mo `diagram.json`?

Wokwi local trong repo nay khong chay tu `diagram.json` mot cach doc lap. File [wokwi.toml](../wokwi.toml) dang tro toi:

- `.pio/build/esp32dev/firmware.bin`
- `.pio/build/esp32dev/firmware.elf`

Vi vay:

- Neu firmware da build thanh cong va ban khong sua code tu lan build gan nhat, ban co the mo `diagram.json` va chay simulator truc tiep.
- Neu vua sua code, vua `clean`, hoac `.pio/build/esp32dev` chua ton tai, ban phai build lai truoc.

Lenh an toan nhat:

```powershell
pio run
```

Roi moi mo `diagram.json` va chay `Wokwi: Start Simulator`.

## 2. So lieu firmware size de dien vao C08 (2.4)

Da do duoc bang `pio run -t size` tren cung project hien tai.

Ket qua:

```text
text    240861
data     87864
bss       5169
dec     333894
hex      51846
```

Y nghia:

- `data + bss` = RAM runtime da dung
- `text + data` = flash image thuong duoc dung de tham khao kich thuoc firmware

So dung de dien vao `C08`:

- `RAM used = 87864 + 5169 = 93033 bytes`
- `93033 bytes = 90.9 KiB`
- ESP32 Dev Module trong PlatformIO bao `320 KB RAM`
- `93033 / 327680 = 28.4% SRAM`

Noi dung nen dien vao bao cao:

> Voi cau hinh firmware hien tai, he thong su dung khoang 93,033 byte RAM (`data + bss`), tuong duong khoang 90.9 KiB, chiem xap xi 28.4% tong SRAM 320 KB cua ESP32 Dev Module. Muc nay van nam duoi nguong 30% da dat ra cho rang buoc C08, de lai du bo nho cho stack, heap va cac buffer I2C/Serial khi he thong van hanh lien tuc.

Neu muon ghi them flash cho dep bao cao:

- `Flash image ~= text + data = 328725 bytes = 321.0 KiB`

## 3. Trang thai thuc te cua bao cao hien tai

Bao cao chua dat muc "100%" vi con 2 diem ho:

- Chua co so lieu do that trong Chuong 5 va doan ket luan ve `test_to_result_ms`, `sampleStdDev`, `passReadyToUnlockMs`.
- Muc `5.6` dang bi thieu han trong muc luc va noi dung. Muc luc nhay tu `5.5.4` sang `5.7`.

## 4. Blocker ve Wokwi headless trong moi truong hien tai

Trong phien lam viec nay:

- Co Wokwi VS Code extension local.
- Khong co `wokwi-cli`.
- Khong co cong cu headless/scriptable nao lo dien ra shell de bat simulator va doc Serial Monitor tu dong.

Vay nen toi khong the thu serial log that cua 3 kich ban ngay trong session nay.

Day la blocker cu the, khong phai do firmware.

## 5. Ban can copy gi tu Wokwi Serial Monitor

### Kich ban 1: Boot / Preheat

Ban can copy:

- cac dong `[BOOT]`
- dong state chuyen sang `STANDBY_LOCKED` sau preheat

Canh dung:

```text
[... ms] BOOT: Alcohol Interlock controller starting
[... ms] BOOT: Mode: ESP32 Arduino + PlatformIO + Wokwi local
[... ms] BOOT: Firmware source of truth: src/*
[... ms] STATE: Transition -> PREHEAT ...
[... ms] STATE: Transition -> STANDBY_LOCKED ...
```

Gia tri co the rut ra:

- thoi gian preheat thuc te ~= moc `STANDBY_LOCKED` - moc boot dau tien

### Kich ban 2: PASS roi START

Thao tac:

1. Vang potentiometer xuong duoi threshold.
2. Nhan `TEST`.
3. Cho he thong vao `PASS_READY`.
4. Nhan `START`.

Ban can copy it nhat cac dong nay:

```text
[... ms] SAMPLE: Window started | count=20 | duration=1000 ms | interval=52 ms | threshold=2000
[... ms] RESULT: Sampling finished in ... ms
[... ms] RESULT: Average ADC=... | stddev=... | threshold=2000 | samples=20/20 | result=PASS
[... ms] METRIC: test_to_result_ms=... | consecutive_fail_count=...
[... ms] METRIC: pass_ready_to_unlock_ms=... | start_to_unlock_ms=...
```

So can dien vao bao cao:

- `testToResultMs`
- `sampleStdDev`
- `passReadyToUnlockMs`
- `startToUnlockMs`
- `Average ADC`

### Kich ban 3: FAIL roi khong cho START

Thao tac:

1. Vang potentiometer len tren threshold.
2. Nhan `TEST`.
3. Nhan `START` de xac nhan he thong khong mo khoa.

Ban can copy it nhat:

```text
[... ms] SAMPLE: Window started | count=20 | duration=1000 ms | interval=52 ms | threshold=2000
[... ms] RESULT: Sampling finished in ... ms
[... ms] RESULT: Average ADC=... | stddev=... | threshold=2000 | samples=20/20 | result=FAIL
[... ms] METRIC: test_to_result_ms=... | consecutive_fail_count=...
```

Dieu ban can ghi ro trong bao cao:

- He thong khong sinh dong `pass_ready_to_unlock_ms` trong kich ban FAIL.
- START bi tu choi, khong chuyen sang `RUNNING`.

## 6. Phan 5.4.1 can bo sung so do nao

Bang 5.4.1 hien dang dung ve mat chi so. Cai con thieu la du lieu do that.

Nen bo sung them cot `Gia tri do duoc` hoac chen mot bang tong hop sau 5.4.1:

| Ma | Chi so | Boot/Preheat | PASS->START | FAIL->no START |
|---|---|---:|---:|---:|
| M01 | testToResultMs | N/A | ... ms | ... ms |
| M02 | passReadyToUnlockMs | N/A | ... ms | N/A |
| M03 | startToUnlockMs | N/A | ... ms | N/A |
| M04 | sampleStdDev | N/A | ... | ... |
| M06 | sampleCount/sampleTotal | N/A | 20/20 | 20/20 |

## 7. Phan 5.6 nen them gi

Tieu de nen them:

`5.6. Ket qua do dac thuc te tu Serial log`

Noi dung goi y de dan:

> De chuyen tu muc "ke hoach kiem thu" sang "co du lieu kiem thu thuc nghiem", nhom da thu thap Serial log truc tiep tu Wokwi local cho ba kich ban chinh: Boot/Preheat, PASS roi START, va FAIL roi khong cho START. Cac metric duoc doi chieu truc tiep voi telemetry cua firmware gom `test_to_result_ms`, `sampleStdDev`, `passReadyToUnlockMs`, `startToUnlockMs` va `sampleCount/sampleTotal`.
>
> O kich ban PASS roi START, firmware hoan tat cua so lay mau sau `... ms`, gia tri trung binh ADC dat `...`, do lech chuan `...`, va he thong chuyen sang `PASS_READY` dung theo thiet ke. Sau khi nguoi dung nhan START hop le, metric `pass_ready_to_unlock_ms` do duoc la `... ms`, con `start_to_unlock_ms` la `... ms`.
>
> O kich ban FAIL roi khong cho START, cua so lay mau hoan tat sau `... ms`, gia tri trung binh ADC vuot nguong 2000, `sampleStdDev` la `...`, va he thong duy tri `FAIL_LOCKED` ma khong mo khoa khi nhan START.
>
> O kich ban Boot/Preheat, he thong khoi dong on dinh, hien thi dung trang thai lam nong va chuyen sang `STANDBY_LOCKED` sau khoang `... ms`, phu hop voi cau hinh `kPreheatMs = 10000`.

## 8. Doan ket luan can thay bang so that

Trong phan `KET LUAN`, doan nay dang con chung chung:

> Cac chi so do dac tu Serial log nhu test_to_result_ms, sampleStdDev va passReadyToUnlockMs xac nhan he thong hoat dong dung theo thiet ke trong moi truong mo phong.

Nen doi thanh cau co so that, vi du:

> Cac chi so do dac tu Serial log xac nhan he thong hoat dong dung theo thiet ke trong moi truong mo phong. Trong kich ban PASS, `testToResultMs` do duoc la `... ms`, `sampleStdDev` la `...`, `passReadyToUnlockMs` la `... ms` va `startToUnlockMs` la `... ms`. Trong kich ban FAIL, `testToResultMs` la `... ms` va he thong duy tri `FAIL_LOCKED`, khong cho phep mo khoa.

## 9. Lenh toi da dung

Lay firmware size:

```powershell
subst X: "D:\PTIT\Embedded System alcohol_interlock"
Set-Location X:\
pio run -t size
subst X: /d
```

Ghi chu:

- Workaround `subst` duoc dung vi PlatformIO tren may hien tai bi loi link ngau nhien khi build trong thu muc co dau cach.
- Do do, neu `pio run` o duong dan goc loi local, ban co the build bang drive tam khong dau cach roi mo Wokwi nhu binh thuong.
