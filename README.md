# hisi-nve
#### Android PoC to read/write Huawei's *NVME* image
----

### Disclaimers
* Use this tool at your own risk and always backup NVME.
* This tool was made for educational purposes only.
* This tool requires a **ROOT shell**.

### Usage
```bash
hisi-nve <w/r> <name> [data]
```
###### (sample outputs captured on a Huawei P10 Lite via [adb](https://developer.android.com/studio/command-line/adb))
```console
# Change the bootloader unlock code to 0123456789ABCDEF
warsaw:/ # ./data/local/tmp/hisi-nve w USRKEY 0123456789ABCDEF
[+] Sucessfully updated USRKEY!
warsaw:/ # reboot bootloader
user@host:~$ fastboot oem unlock 0123456789ABCDEF
...
(bootloader) The device will reboot and do factory reset...
OKAY [  2.415s]
finished. total time: 2.416s
user@host:~$

# Read the S/N of the phone
warsaw:/ # ./data/local/tmp/hisi-nve r SN
[+] 2XJDU17408002XXX
warsaw:/ #
```

### Compilation
For convenience, a `Makefile` is included to work with the cross-compiler toolchain from Android NDK (*r19* or higher).
```console
# Download the Android NDK.
user@host:~$ wget https://dl.google.com/android/repository/android-ndk-r20-linux-x86_64.zip

# Extract the NDK and append it to the $PATH.
user@host:~$ unzip android-ndk-r20-linux-x86_64.zip
user@host:~$ rm android-ndk-r20-linux-x86_64.zip  # optional
user@host:~$ export PATH=$PATH:~/android-ndk-r20

# Clone the `hisi-nve` git repository and build the binaries.
user@host:~$ git clone https://github.com/R0rt1z2/hisi-nve.git && cd hisi-nve
user@host:~/hisi-nve$ make && file obj/local/{arm64-v8a,armeabi-v7a}/hisi-nve
obj/local/arm64-v8a/hisi-nve:   ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /system/bin/linker64, BuildID[sha1]=X, with debug_info, not stripped
obj/local/armeabi-v7a/hisi-nve: ELF 32-bit LSB shared object, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /system/bin/linker, BuildID[sha1]=X, with debug_info, not stripped
user@host:~/hisi-nve$
```

## Supported SoCs
Name | Code Name | Read | Write
------ | ----- | ----- | -----
Kirin 620 | `hi6210sft` | Yes | Yes
Kirin 65X | `hi6250` | Yes | Yes
Kirin 960 | `hi3660` | Yes | Yes
Kirin 950 | `kirin950` | ? | ?
Kirin 970 | `kirin970` | ? | ?
Kirin 710 | `kirin710` | Yes | No
Kirin 980 | `kirin980` | ? | ?
Kirin 990 | `kirin990` | ? | ?


### Adding support for new SoCs
For convenience, a python3 script (`src/utils/get_nve_entries.py`) has been provided to automatically extract all entries from the desired NVME image into a `hardware_range mapping` table:

1. Extract all the nvme entries into a header file

```console
user@host:~$ python3 src/utils/get_nve_entries.py nvme-kirin980.img kirin980 # (kirin980 is the ro.hardware of the phone)
Sucessfully extracted 402 entries from nvme-kirin980.img to kirin980.h!
user@host:~$
````

2. Apply the new nvme mappings to `nve_mappings.h`
```patch
--- src/include/nve/nve_mappings.h  2022-01-30 21:21:55.005061600 +0100
+++ src/include/nve/nve_mappings.h" 2022-01-30 21:22:33.411245600 +0100
@@ -1,7 +1,7 @@
 #ifndef NVE_MAPPINGS_H
 #define NVE_MAPPINGS_H

-#define NVE_SUPPORTED_SOCS 5
+#define NVE_SUPPORTED_SOCS 6

 struct hardware_range {
     char *soc_name;
@@ -1911,12 +1911,26 @@
     }
 };

-static struct hardware_range *all_ranges[5] = {
+// Automatically generated mapping table for kirin980
+static struct hardware_range kirin980_ranges = {
+    .soc_name = "kirin980",
+    .nve_entry_count = 402,
+    .nve_hashed_key = 1,
+    .nve_entries = {
+        "SWVERSI",
+        "SN",
+        ...
+        "WICCKC1"
+    }
+};
+
+static struct hardware_range *all_ranges[6] = {
     &hi6210sft_ranges,
     &hi6250_ranges,
     &kirin950_ranges,
     &kirin970_ranges,
-    &kirin710_ranges
+    &kirin710_ranges,
+    &kirin980_ranges
 };

 #endif // NVE_MAPPINGS_H
```
