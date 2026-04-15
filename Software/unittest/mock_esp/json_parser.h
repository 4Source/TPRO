#pragma once

#include <cstdint>
#include <cstring>

// ESP-IDF Typen simulieren
typedef int32_t esp_err_t;
#define OS_SUCCESS 0
#define OS_FAIL -1

// Die Struktur, die dein Code erwartet
typedef struct {
	const char *json;
	size_t length;
	// Hier kannst du interne Zustände für Tests hinzufügen, falls nötig
} jparse_ctx_t;

// Mock-Implementierungen der Funktionen
inline esp_err_t json_parse_start(jparse_ctx_t *ctx, const char *json, size_t len) {
	ctx->json = json;
	ctx->length = len;
	// Simpler Check: Wenn der String leer oder kein JSON ist, schlage fehl
	if (!json || json[0] != '{')
		return OS_FAIL;
	return OS_SUCCESS;
}

inline esp_err_t json_obj_get_object(jparse_ctx_t *ctx, const char *name) {
	// Prüft nur, ob der Key im String vorkommt (simpelster Mock)
	if (strstr(ctx->json, name) != nullptr)
		return OS_SUCCESS;
	return OS_FAIL;
}

inline esp_err_t json_obj_get_string(jparse_ctx_t *ctx, const char *name, char *buffer, size_t buf_size) {
	// 1. Suche nach dem Key (z.B. "type")
	const char *key_pos = strstr(ctx->json, name);
	if (!key_pos)
		return OS_FAIL;

	// 2. Suche den Doppelpunkt nach dem Key
	const char *colon = strchr(key_pos, ':');
	if (!colon)
		return OS_FAIL;

	// 3. Suche das öffnende Anführungszeichen des Wertes NACH dem Doppelpunkt
	const char *start = strchr(colon, '\"');
	if (!start)
		return OS_FAIL;
	start++; // Wir wollen den Inhalt NACH dem "

	// 4. Suche das schließende Anführungszeichen
	const char *end = strchr(start, '\"');
	if (!end)
		return OS_FAIL;

	// 5. Kopieren und Null-Terminieren
	size_t len = end - start;
	if (len >= buf_size)
		len = buf_size - 1;

	memcpy(buffer, start, len);
	buffer[len] = '\0';

	return OS_SUCCESS;
}

inline esp_err_t json_parse_end(jparse_ctx_t *ctx) { return OS_SUCCESS; }