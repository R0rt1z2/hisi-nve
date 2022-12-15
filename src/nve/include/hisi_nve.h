#pragma once

#define NVE_INVALID_NVM             0xFFFFFFFF
#define NVE_BASE_VERSION            0x1
#define NVE_CRC_SUPPORT_VERSION     0x2
#define NVE_PARTITION_INVALID_AGE   0x0

#define NVE_PARTITION_COUNT     8
#define NV_ITEMS_MAX_NUMBER     1023
#define NVE_BLOCK_SIZE	        512

#define NVE_PARTITION_SIZE   	(128 * 1024)
#define NVE_PARTITION_NUMBER 	7 /* (mmcblk0p7) */

#define NV_CMDLINE_PATH         "/proc/cmdline"
#define NV_CMDLINE_LEN          2048

#define NV_IOCTL_NODE           "/dev/nve0"
#define NVME_IOCTL_ACCESS_DATA  _IOWR('M', 25, struct hisi_nve_info_user) /* NVEACCESSDATA */

#define NV_INFO_LEN     1024
#define NVE_HEADER_NAME "Hisi-NV-Partition" /* reliable data area */

#define NV_NAME_LENGTH   8   /* nve name maximum length */
#define NVE_NV_DATA_SIZE 104 /* nve data maximum length */

#define NVE_NV_USRLOCK_LEN   16  /* USRLOCK maximum length */

#define NV_WRITE 0 	/* nve write operation*/
#define NV_READ  1  /* nve read  operation*/

#define NVE_NV_HASHED_KEY_MAX_LEN  16 /* frpkey and usrkey */

#define NVE_NV_CRC_HEADER_SIZE  20
#define NVE_NV_CRC_DATA_SIZE    104
#define PARTITION_HEADER_SIZE   128

#define PARTITION_HEADER_OFFSET (NVE_PARTITION_SIZE - PARTITION_HEADER_SIZE)

#define NVE_ERROR_NO_MEM     1
#define NVE_ERROR_PARAM      2
#define NVE_ERROR_PARTITION  3

 struct hisi_nve_info_user {
	uint32_t nv_operation;
	uint32_t nv_number;
	char nv_name[NV_NAME_LENGTH];
	uint32_t valid_size;
	unsigned char nv_data[NVE_NV_DATA_SIZE];
};

struct NVE_partition_header {
	char nve_partition_name[32];
	unsigned int nve_version;
	unsigned int nve_block_id;
	unsigned int nve_block_count;
	unsigned int valid_items;
	unsigned int nv_checksum;
	unsigned int nve_crc_support;
	unsigned char reserved[68];
	unsigned int nve_age;
};

struct NV_items_struct {
	unsigned int nv_number;
	char nv_name[NV_NAME_LENGTH];
	unsigned int nv_property;
	unsigned int valid_size;
	unsigned int crc;
	char nv_data[NVE_NV_DATA_SIZE];
};

struct NVE_partittion {
	struct NV_items_struct NV_items[NV_ITEMS_MAX_NUMBER];
	struct NVE_partition_header header;
};

struct NVE_struct {
	int nve_major_number;
	int initialized;
	unsigned int nve_partition_count;
	unsigned int nve_current_id;
	struct NVE_partittion *nve_current_ramdisk;
	struct NVE_partittion *nve_update_ramdisk;
	struct NVE_partittion *nve_store_ramdisk;
};

char *nvme_paths[] = {
        "/dev/block/mmcblk0p7",
        "/dev/block/platform/hi_mci.0/by-name/nvme",
        "/dev/block/bootdevice/by-name/nvme",
        NULL
};