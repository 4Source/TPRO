#include "esp_http_server.h"
#include "websocket_server.hpp"
#include <gtest/gtest.h>

TEST(WebsocketServerTest, SendFrameDataCorrectly) {
	LedFrame frame;
	// Testwerte
	for (int x = 0; x < LedFrame::kWidth; x++) {
		for (int y = 0; y < LedFrame::kHeight; y++) {
			frame.led_data[x][y] = {1, 2, 3};
		}
	}

	// Fake-Server-Handle
	httpd_handle_t fake_server = reinterpret_cast<httpd_handle_t>(0x1234);
	WebsocketServer ws_server(fake_server, frame);

	esp_err_t err = ws_server.update_simulation();

	EXPECT_EQ(err, ESP_OK);

	// WIDTH * HEIGHT * (3 Bytes pro Pixel)
	size_t expected_size = LedFrame::kWidth * LedFrame::kHeight * 3;
	EXPECT_EQ(last_send_len, expected_size);

	// KOmmt erstes Pixel korrekt an
	ASSERT_NE(last_send_payload, nullptr);
	EXPECT_EQ(last_send_payload[0], 1); // Red
	EXPECT_EQ(last_send_payload[1], 2); // Green
	EXPECT_EQ(last_send_payload[2], 3); // Blue
}