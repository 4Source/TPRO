#ifndef CJSON_H
#define CJSON_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Typ-Konstanten (Wichtig für IsArray, IsNumber etc.) ---
#define cJSON_Invalid (0)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array (1 << 5)
#define cJSON_Object (1 << 6)
extern const char *global_current_json_buffer;
typedef struct cJSON {
	struct cJSON *next, *prev, *child;
	int type;
	char *valuestring;
	int valueint;
	double valuedouble;
	char *string;
} cJSON;

// --- Navigation Makro ---
#define cJSON_ArrayForEach(element, array) for (element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

// --- Typ-Checks ---
static inline int cJSON_IsArray(const cJSON *const item) { return (item && (item->type & cJSON_Array)); }
static inline int cJSON_IsObject(const cJSON *const item) { return (item && (item->type & cJSON_Object)); }
static inline int cJSON_IsNumber(const cJSON *const item) { return (item && (item->type & cJSON_Number)); }
static inline int cJSON_IsString(const cJSON *const item) { return (item && (item->type & cJSON_String)); }

// --- Memory Management ---
static inline void cJSON_Delete(cJSON *c) {
	if (!c)
		return;
	cJSON *child = c->child;
	while (child) {
		cJSON *next = child->next;
		cJSON_Delete(child);
		child = next;
	}
	if (c->string)
		free(c->string);
	if (c->valuestring)
		free(c->valuestring);
	free(c);
}

// --- Konstruktoren ---
static inline cJSON *cJSON_Parse(const char *v) { return (cJSON *)calloc(1, sizeof(cJSON)); }

static inline cJSON *cJSON_CreateObject(void) {
	cJSON *n = (cJSON *)calloc(1, sizeof(cJSON));
	if (n)
		n->type = cJSON_Object;
	return n;
}

static inline cJSON *cJSON_CreateArray(void) {
	cJSON *n = (cJSON *)calloc(1, sizeof(cJSON));
	if (n)
		n->type = cJSON_Array;
	return n;
}

static inline cJSON *cJSON_CreateNumber(double n) {
	cJSON *item = (cJSON *)calloc(1, sizeof(cJSON));
	if (item) {
		item->type = cJSON_Number;
		item->valuedouble = n;
		item->valueint = (int)n;
	}
	return item;
}

// --- Baum-Operationen ---
static inline void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
	if (!array || !item)
		return;
	if (!array->child) {
		array->child = item;
	} else {
		cJSON *c = array->child;
		while (c->next)
			c = c->next;
		c->next = item;
		item->prev = c;
	}
}

static inline void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
	if (!object || !string || !item)
		return;
	if (item->string)
		free(item->string);
	item->string = strdup(string);
	cJSON_AddItemToArray(object, item);
}

static inline int cJSON_GetArraySize(const cJSON *array) {
	int size = 0;
	if (array) {
		cJSON *c = array->child;
		while (c) {
			size++;
			c = c->next;
		}
	}
	return size;
}

// Hilfsfunktion: Sucht einen Wert direkt im rohen JSON-Text
static inline char *mock_extract_value(const char *key) {
	// Hier greifen wir auf den aktuellen JSON-String zu.
	// In jparse_ctx_t ctx->json gespeichert. Wir nehmen hier einen globalen Pointer
	// oder einen Trick, um an den String zu kommen:
	extern const char *global_current_json_buffer;
	if (!global_current_json_buffer)
		return NULL;

	const char *p = strstr(global_current_json_buffer, key);
	if (!p)
		return NULL;

	p = strchr(p, ':');
	if (!p)
		return NULL;
	p++; // Hinter den Doppelpunkt

	// Überspringe Leerzeichen und Anführungszeichen
	while (*p == ' ' || *p == '\"' || *p == '\t')
		p++;

	// Kopiere den Wert bis zum Ende (Anführungszeichen, Komma oder Klammer)
	const char *end = p;
	while (*end && *end != '\"' && *end != ',' && *end != '}' && *end != ']')
		end++;

	size_t len = end - p;
	char *res = (char *)malloc(len + 1);
	strncpy(res, p, len);
	res[len] = '\0';
	return res;
}

static inline cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
	if (!object || !string)
		return NULL;

	// 1. Schauen, ob wir das Item schon gemockt haben
	cJSON *c = object->child;
	while (c) {
		if (c->string && strcmp(c->string, string) == 0)
			return c;
		c = c->next;
	}

	// 2. Neues Item erstellen
	cJSON *dummy = (cJSON *)calloc(1, sizeof(cJSON));
	dummy->string = strdup(string);
	cJSON_AddItemToArray((cJSON *)object, dummy);

	// 3. DATEN-EXTRAKTION AUS DEM JSON-STRING
	char *val = mock_extract_value(string);
	if (val) {
		if (string[0] == 'i' || string[0] == 'c') { // id, cycle_time
			dummy->type = cJSON_Number;
			dummy->valueint = atoi(val);
			dummy->valuedouble = (double)dummy->valueint;
		} else {
			dummy->type = cJSON_String;
			dummy->valuestring = val; // String wird übernommen
		}
		if (string[0] != 'i' && string[0] != 'c') {
			// val wurde als valuestring übernommen, nicht free'n
		} else {
			free(val);
		}
	}

	// 4. STRUKTUR-LOGIK (Arrays/Objekte triggern)
	if (strcmp(string, "secondary") == 0 || strcmp(string, "subeffects") == 0 || strcmp(string, "overwrite") == 0) {
		dummy->type = (strcmp(string, "overwrite") == 0) ? cJSON_Object : cJSON_Array;
		// Wir erstellen EIN Kind-Element, damit dein Code in die Schleife geht
		cJSON *child = (cJSON *)calloc(1, sizeof(cJSON));
		child->type = cJSON_Object;
		if (strcmp(string, "overwrite") == 0) {
			// Für Overwrites brauchen wir einen Key ("color")
			child->string = strdup("color");
			child->type = cJSON_String;
			child->valuestring = strdup("#FF0000");
		}
		cJSON_AddItemToArray(dummy, child);
	}

	return dummy;
}

// --- Output ---
static inline char *cJSON_PrintUnformatted(const cJSON *item) {
	const char *fake_json = "{\"type\": \"blink\"}";
	char *res = (char *)malloc(strlen(fake_json) + 1);
	if (res)
		strcpy(res, fake_json);
	return res;
}

// --- Hilfs-Makros ---
#define cJSON_AddStringToObject(o, n, s) // Aktuell leer, da wir im Mock meist nur lesen
#define cJSON_AddNumberToObject(o, n, num) cJSON_AddItemToObject(o, n, cJSON_CreateNumber(num))
#define cJSON_AddObjectToObject(o, n)                                                                                                                \
	({                                                                                                                                               \
		cJSON *obj = cJSON_CreateObject();                                                                                                           \
		cJSON_AddItemToObject(o, n, obj);                                                                                                            \
		obj;                                                                                                                                         \
	})
#define cJSON_AddArrayToObject(o, n)                                                                                                                 \
	({                                                                                                                                               \
		cJSON *arr = cJSON_CreateArray();                                                                                                            \
		cJSON_AddItemToObject(o, n, arr);                                                                                                            \
		arr;                                                                                                                                         \
	})

#ifdef __cplusplus
}
#endif
#endif