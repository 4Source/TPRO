import './style.css';

declare const __APP_VERSION__: string;

export function About() {
	return (
		<div class="about">
			<h1>Light Wall</h1>
			<p class="about-subtitle">LED-Matrix Controller</p>

			<div class="about-card">
				<div class="about-row">
					<span class="about-label">Version</span>
					<span class="about-value">{__APP_VERSION__}</span>
				</div>
				<div class="about-row">
					<span class="about-label">Platform</span>
					<span class="about-value">ESP32-S3</span>
				</div>
				<div class="about-row">
					<span class="about-label">Matrix</span>
					<span class="about-value">87 × 35 LEDs (3 045 gesamt)</span>
				</div>
				<div class="about-row">
					<span class="about-label">Framework</span>
					<span class="about-value">ESP-IDF · Preact</span>
				</div>
			</div>
		</div>
	);
}
