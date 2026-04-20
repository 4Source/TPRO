import { render } from 'preact';
import { LocationProvider, Router, Route } from 'preact-iso';

import { Header } from './components/Header/Header';
import { Home } from './pages/Home/index';
import { About } from './pages/About/index';
import { Simulation } from './pages/Simulation';
import { NotFound } from './pages/_404';
import './style.css';
import { Files } from './pages/Files';

export function App() {
	return (
		<LocationProvider>
			<Header />
			<main>
				<Router>
					<Route path="/" component={Home} />
					<Route path="/about" component={About} />
					<Route path="/simulation" component={Simulation} />
					<Route path="/browser" component={Files} />
					<Route default component={NotFound} />
				</Router>
			</main>
		</LocationProvider>
	);
}

render(<App />, document.getElementById('app'));
