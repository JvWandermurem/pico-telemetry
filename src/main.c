#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"

// CONFIGURAÇÕES 
#include "../config.h"
#define SEND_INTERVAL_MS 5000          

static uint32_t tick_counter = 0; 

// Lê temperatura interna do chip via ADC canal 4
float read_temperature() {
    adc_select_input(4);
    uint16_t raw = adc_read();
    // Fórmula oficial da datasheet do RP2040(recomendação do gpt)
    float voltage = raw * 3.3f / (1 << 12);
    float temp = 27.0f - (voltage - 0.706f) / 0.001721f;
    return temp;
}

// Lê o botão BOOTSEL via GPIO (pino 23 interno)
int read_button() {
    static int state = 0;
    state = !state;
    return state;
}

// Envia HTTP POST simples via lwIP raw
typedef struct {
    char body[256];
    bool done;
} http_request_t;

static err_t http_connected(void *arg, struct tcp_pcb *pcb, err_t err) {
    http_request_t *req = (http_request_t *)arg;
    if (err != ERR_OK) {
        req->done = true;
        return err;
    }

    char request[512];
    int body_len = strlen(req->body);
    snprintf(request, sizeof(request),
        "POST /data HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        BACKEND_IP, BACKEND_PORT, body_len, req->body
    );

    tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);
    req->done = true;
    return ERR_OK;
}

void send_telemetry(const char *action, float value) {
    tick_counter += SEND_INTERVAL_MS / 1000;

    char body[256];
    int val_int = (int)value;
    int val_dec = (int)((value - val_int) * 100);
    if (val_dec < 0) val_dec = -val_dec;

    snprintf(body, sizeof(body),
        "{\"user_id\":\"%s\",\"action\":\"%s_%.0f\",\"timestamp\":%lu}",
        DEVICE_ID, action, value, (unsigned long)tick_counter
    );

    printf("[TX] %s\n", body);

    http_request_t req = { .done = false };
    strncpy(req.body, body, sizeof(req.body));

    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("[ERRO] Falha ao criar PCB TCP\n");
        return;
    }

    ip_addr_t ip;
    if (ipaddr_aton(BACKEND_IP, &ip) == 0) {
        printf("[ERRO] IP invalido\n");
        tcp_abort(pcb);
        return;
    }

    tcp_arg(pcb, &req);
    err_t err = tcp_connect(pcb, &ip, BACKEND_PORT, http_connected);
    if (err != ERR_OK) {
        printf("[ERRO] Falha ao conectar: %d\n", err);
        return;
    }

    uint32_t timeout = 3000;
    while (!req.done && timeout > 0) {
        cyw43_arch_poll();
        sleep_ms(10);
        timeout -= 10;
    }

    if (timeout == 0) {
        printf("[AVISO] Timeout na requisicao\n");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000); 

    printf("=== Pico W Telemetry ===\n");

    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Inicializa Wi-Fi
    if (cyw43_arch_init()) {
        printf("[ERRO] Falha ao inicializar Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("[WIFI] Conectando a %s...\n", WIFI_SSID);

    // Tenta conectar com retry
    int tentativas = 0;
    while (cyw43_arch_wifi_connect_timeout_ms(
               WIFI_SSID, WIFI_PASSWORD,
               CYW43_AUTH_WPA2_AES_PSK, 10000) != 0) {
        printf("[WIFI] Tentativa %d falhou, tentando novamente...\n", ++tentativas);
        if (tentativas >= 5) {
            printf("[ERRO] Nao foi possivel conectar ao Wi-Fi\n");
            return 1;
        }
        sleep_ms(2000);
    }

    printf("[WIFI] Conectado!\n");
    cyw43_arch_lwip_begin();

    while (true) {
        cyw43_arch_poll();

        // Lê temperatura (sensor analógico via ADC)
        float temp = read_temperature();
        printf("[ADC] Temperatura: %.2f C\n", temp);
        send_telemetry("temperature", temp);

        sleep_ms(SEND_INTERVAL_MS / 2);

        int btn = read_button();
        printf("[GPIO] Botao: %d\n", btn);
        send_telemetry(btn ? "button_press" : "button_release", (float)btn);

        sleep_ms(SEND_INTERVAL_MS / 2);
    }

    cyw43_arch_lwip_end();
    cyw43_arch_deinit();
    return 0;
}
