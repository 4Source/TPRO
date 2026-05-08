import { useLocation } from 'preact-iso';
import './style.css';

export function Header() {
	const { url } = useLocation();

	return (
		<header>
			<nav>
				<a href="/simulation" class={url === '/simulation' || url === '/' ? 'active' : ''}>
					Simulation
				</a>
				<a href="/browser" class={url === '/browser' ? 'active' : ''}>
					File Server
				</a>
				<a href="/about" class={url === '/about' ? 'active' : ''}>
					About
				</a>
			</nav>
		</header>
	);
}
