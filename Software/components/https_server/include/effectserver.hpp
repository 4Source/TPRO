#pragma once
#include "light_effect_manager.hpp"
#include <esp_https_server.h>

class EffectServer {
  public:
	inline static LightEffectManager *manager = nullptr;

	// PUT /effect/<fs-path>  – Body enthält neues JSON, Datei wird gespeichert
	// und der Effekt im Manager neu geladen.
	static esp_err_t handle_put_effect(httpd_req_t *req);

	static esp_err_t init(LightEffectManager &mgr);
};
