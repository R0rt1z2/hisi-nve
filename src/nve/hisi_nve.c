#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "crypto/sha256.h"
#include "nve/hisi_nve.h"
#include "nve/nve_mappings.h"

struct hardware_range *nve_range;

uint32_t get_nve_index_with_name(char *nve_name) {
    for (uint32_t nve_index = 0; nve_index < nve_range->nve_entry_count; nve_index++) {
        if (nve_range->nve_entries[nve_index] != NULL 
            && strcmp(nve_name, nve_range->nve_entries[nve_index]) == 0) {
                return nve_index;
        }
    }
    return -1;
}

struct hardware_range *get_hardware_range(char *soc) {
    for (int i = 0; i < sizeof(all_ranges); i++) {
        if (!strcmp(soc, all_ranges[i]->soc_name)) {
            return all_ranges[i];
        }
    }
    NVE_ERROR("Unsupported CPU: %s!\n", soc);
}

char *get_nve_hisi_soc_name() {
    char cmdline[NV_CMDLINE_LEN];
    char *nve_hisi_soc = malloc(10);

    FILE *fp = fopen(NV_CMDLINE_PATH, "r");
    if(fp == NULL)
        NVE_ERROR("Could not open %s!\n", NV_CMDLINE_PATH);

    while(fgets(cmdline, NV_CMDLINE_LEN, fp) != NULL) {
        int length = strlen(cmdline);
        cmdline[length - 1] = cmdline[length - 1] == '\n' 
                ? '\0' : cmdline[length - 1];
    } 

    char *match = strstr(cmdline, "androidboot.hardware=");
    if (match == NULL)
        NVE_ERROR("Could not read hardware!\n");

    long offset = ((match - cmdline) + strlen("androidboot.hardware="));
    for (int i = 0; i < (strlen(cmdline) - offset); i++) {
        if (isspace(cmdline[offset + i])) break;
        snprintf(nve_hisi_soc, 10, "%s%c", nve_hisi_soc, cmdline[offset + i]);
    }

    return nve_hisi_soc;
}

int main(int argc, char *argv[]) {
    uint32_t rw = 0, hash = 0;
    char hashed_key[SHA256_BLOCK_SIZE];
    
    nve_range = get_hardware_range(get_nve_hisi_soc_name());

    if (argc < 3 || *argv[1] == 'h')
        NVE_ERROR("Usage command: %s <w/r> <name> [data]\n", argv[0]);

    argv++;

    if (*argv[0] != 'r' && *argv[0] != 'w')
        NVE_ERROR("Invalid operation: %s!\n", argv[0]);

    if (*argv[0] == 'r')
        rw = 1;

    if (sizeof(argv[1]) > NV_NAME_LENGTH 
        || get_nve_index_with_name(argv[1]) == -1)
            NVE_ERROR("Wrong nv item name: %s!\n", argv[1]);

    if (!strcmp(argv[1], "USRKEY")
        && nve_range->nve_hashed_key != 0)
            hash = 1;

    int nvme_fd = open(NV_IOCTL_NODE, O_RDWR);

    if (nvme_fd < 0)
        NVE_ERROR("Could not open: %s!\n", NV_IOCTL_NODE);

    struct hisi_nve_info_user nveCommandBuffer = {
        .nv_operation  = rw,
        .nv_number     = get_nve_index_with_name(argv[1]),
        .valid_size    = NVE_NV_DATA_SIZE
    };

    strncpy(nveCommandBuffer.nv_name, 
            argv[1], (sizeof(argv[2]) - 1));
    nveCommandBuffer.nv_name[(sizeof(argv[2])) - 1] = '\0';

    if (!rw) {
        if (hash) {
            if (strlen(argv[2]) > NVE_NV_USRLOCK_LEN)
                NVE_ERROR("Invalid unlock code: %s!\n", argv[2]);
            SHA256(argv[2], strlen(argv[2]), hashed_key);
            strncpy(nveCommandBuffer.nv_data, hashed_key, (strlen(hashed_key)));
        } else {
            strncpy(nveCommandBuffer.nv_data, 
                argv[2], (sizeof(argv[2]) - 1));
            nveCommandBuffer.nv_data[(sizeof(argv[2])) - 1] = '\0';
        }
    }

    int ret = ioctl(nvme_fd, NVME_IOCTL_ACCESS_DATA, 
                   (struct hisi_nve_info_user*) &nveCommandBuffer);
    
    if (ret < 0)
        NVE_ERROR("Something went wrong: %s!\n", strerror(errno));

    if (rw) {
        NVE_SUCCESS("%s\n", nveCommandBuffer.nv_data);
    } else {
        NVE_SUCCESS("Sucessfully updated %s!\n", argv[1]);
    }

    close(nvme_fd);
    return ret;
}