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
} jparse_ctx_t;

// --- Bestehende Funktionen ---

inline esp_err_t json_parse_start(jparse_ctx_t *ctx, const char *json, size_t len) {
	ctx->json = json;
	ctx->length = len;
	if (!json || (json[0] != '{' && json[0] != '['))
		return OS_FAIL;
	return OS_SUCCESS;
}

inline esp_err_t json_obj_get_object(jparse_ctx_t *ctx, const char *name) {
	if (strstr(ctx->json, name) != nullptr)
		return OS_SUCCESS;
	return OS_FAIL;
}

inline esp_err_t json_obj_get_string(jparse_ctx_t *ctx, const char *name, char *buffer, size_t buf_size) {
	const char *key_pos = strstr(ctx->json, name);
	if (!key_pos)
		return OS_FAIL;
	const char *colon = strchr(key_pos, ':');
	if (!colon)
		return OS_FAIL;
	const char *start = strchr(colon, '\"');
	if (!start)
		return OS_FAIL;
	start++;
	const char *end = strchr(start, '\"');
	if (!end)
		return OS_FAIL;
	size_t len = end - start;
	if (len >= buf_size)
		len = buf_size - 1;
	memcpy(buffer, start, len);
	buffer[len] = '\0';
	return OS_SUCCESS;
}

// --- NEU: Fehlende Funktionen für Timeline/Blinking Effects ---

inline esp_err_t json_obj_get_int(jparse_ctx_t *ctx, const char *name, int *out_value) {
	if (!ctx || !ctx->json || !name || !out_value)
		return OS_FAIL;

	// Wir arbeiten auf einer Kopie des JSON-Strings für die Suche
	const char *current_pos = ctx->json;

	// Pfad-Logik: Wir suchen nacheinander nach den Segmenten (getrennt durch '.')
	char path[256];
	strncpy(path, name, sizeof(path));
	char *segment = strtok(path, ".");

	while (segment != nullptr) {
		// Suche das aktuelle Segment im verbleibenden JSON
		const char *found = strstr(current_pos, segment);
		if (!found)
			return OS_FAIL; // Segment nicht gefunden

		current_pos = found + strlen(segment);
		segment = strtok(nullptr, ".");
	}

	// Wenn wir hier sind, sind wir beim letzten Segment (z.B. "id") angekommen.
	// Jetzt suchen wir den Wert nach dem nächsten Doppelpunkt.
	const char *colon = strchr(current_pos, ':');
	if (!colon)
		return OS_FAIL;

	char *end_ptr;
	long val = strtol(colon + 1, &end_ptr, 10);

	// Validierung: Hat strtol wirklich eine Zahl gelesen?
	if (end_ptr == colon + 1) {
		// Fallback: Eventuell steht die Zahl in Anführungszeichen?
		const char *quote = strchr(colon, '\"');
		if (quote) {
			val = strtol(quote + 1, &end_ptr, 10);
		} else {
			return OS_FAIL;
		}
	}

	*out_value = static_cast<int>(val);
	return OS_SUCCESS;
}

inline esp_err_t json_obj_get_array(jparse_ctx_t *ctx, const char *name, int *out_size) {
	// Simpler Mock: Wenn Key da, täusche ein Array der Größe 3 vor (für RGB)
	if (strstr(ctx->json, name)) {
		if (out_size)
			*out_size = 3;
		return OS_SUCCESS;
	}
	return OS_FAIL;
}

inline esp_err_t json_arr_get_int(jparse_ctx_t *ctx, int index, int *out_value) {
	if (out_value)
		*out_value = 0;
	return OS_SUCCESS;
}

inline esp_err_t json_obj_leave_array(jparse_ctx_t *ctx) { return OS_SUCCESS; }

inline esp_err_t json_parse_end(jparse_ctx_t *ctx) { return OS_SUCCESS; }