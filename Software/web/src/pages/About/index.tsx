import preactLogo from '../../assets/preact.svg';
import worldmapImage from '../../assets/worldmap.svg';
import { Clock } from '../../components/Clock/Clock';
import './style.css';

export function About() {
	return (
		<div class="about">
			<a href="https://preactjs.com" target="_blank" rel="noreferrer">
				<img src={preactLogo} alt="Preact logo" height="160" width="160" />
			</a>
			<h1>This is the about page</h1>
			<img src={worldmapImage} height="180" width="300" />
			<Clock />
		</div>
	);
}
