#include <cstddef>
#include <cstdint>

extern "C" {
size_t last_send_len = 0;
uint8_t *last_send_payload = nullptr;
}