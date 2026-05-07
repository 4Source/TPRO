#pragma once

#include <string>

/**
 * @brief Hilfsstruktur für konfigurierbare Parameter der effekte
 * Wird verwendet um Parameter der Effekte, die über die API gesetzt werden können, zu beschreiben. Zum Beispiel:
 * - type: "number", min: 0, max: 100, step: 1 -> für einen Helligkeitswert
 * Alle nicht benötigten Felder können je nach Typ ignoriert werden.
 * Schema soll Brücke für automatische Web-UI Generierung sein!
 */
struct ParameterSchema {
	std::string type;

	int min = 0;
	int max = 0;
	int step = 1;
};

template <typename T> struct Param {
	T value;
	ParameterSchema schema;
};