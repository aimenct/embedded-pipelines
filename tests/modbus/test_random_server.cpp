// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <modbus.h>

int main(void)
{
    int s = -1, sacc = -1;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;

    ctx = modbus_new_tcp("127.0.0.1", 1502);
    //ctx = modbus_new_tcp("192.168.16.88", 502);
    //ctx = modbus_new_tcp("192.168.16.88", 1502);
    /* modbus_set_debug(ctx, TRUE); */

    mb_mapping = modbus_mapping_new(500, 500, 500, 500);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to create the context: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    for (int i=0; i<500; i = i + 2) {
        float fAux = (float)(65535.0*rand() / (RAND_MAX + 1.0));
        memcpy(&mb_mapping->tab_input_registers[i], &fAux, sizeof(float));
        mb_mapping->tab_input_bits[i] = (uint16_t) (fAux) % 2;

        fAux = (float)(65535.0*rand() / (RAND_MAX + 1.0));
        memcpy(&mb_mapping->tab_registers[i], &fAux, sizeof(float));
        mb_mapping->tab_bits[i] = (uint16_t) (fAux) % 2;
    }

    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    printf("Listening\n");
    s = modbus_tcp_listen(ctx, 1);
    if (s == -1) {
        fprintf(stderr, "Failed to listen: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
    printf("Creating socket\n");
    sacc = modbus_tcp_accept(ctx, &s);
    if (sacc == -1) {
        fprintf(stderr, "Failed to create the connection: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    printf("Starting loop\n");

    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;

        printf("Analizing request\n");
        rc = modbus_receive(ctx, query);

        if (rc > 0) {
            /* rc is the query size */
            printf("Replying\n");
            modbus_reply(ctx, query, rc, mb_mapping);
        } else if (rc == -1) {
            printf("Connection closed by the client or error \n");
            /* Connection closed by the client or error */
            break;
        }

    for (int i=0; i<500; i = i + 2) {
        float faux = (float)(65535.0*rand() / (RAND_MAX + 1.0));
        memcpy(&mb_mapping->tab_input_registers[i], &faux, sizeof(float));
        mb_mapping->tab_input_bits[i] = (uint16_t) (faux) % 2;

        faux = (float)(65535.0*rand() / (RAND_MAX + 1.0));
        memcpy(&mb_mapping->tab_registers[i], &faux, sizeof(float));
        mb_mapping->tab_bits[i] = (uint16_t) (faux) % 2;
    }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));

    if (s != -1) {
        close(s);
    }
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
