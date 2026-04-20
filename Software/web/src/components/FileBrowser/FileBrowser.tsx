import { Component } from 'preact';
import './styles.css';
import { Spinner } from '../Spinner/Spinner';
import { ArrowDownToLine, File, FilePlusCorner, Folder, RotateCcw, Trash2 } from 'lucide-preact';

type FileBrowserState = {
	path: string[];
	pathInput: string;
	entries: string[];
	loading: boolean;
};

// Example for a Component as class with states
export class FileBrowser extends Component<{}, FileBrowserState> {
	fileInput?: HTMLInputElement;

	constructor() {
		super();
		this.state = {
			path: [],
			pathInput: '/',
			entries: [],
			loading: false,
		};
	}

	loadFiles = async () => {
		this.setState({ loading: true });
		let success = false;
		const path = `/directory/${this.state.path.join('')}`;
		await fetch(path, {
			method: 'GET',
		}).then(async res => {
			console.debug(res);
			const content = await res.json();
			if (Array.isArray(content)) {
				content.sort();
				this.setState({ entries: content });
				success = true;
			}
			else {
				console.error('loadFiles: Response is not an array');
			}
		}).catch(reason => {
			console.error(reason);
		}).finally(() => {
			this.setState({ loading: false });
		});
		return success;
	};

	uploadFile = async (file: File) => {
		this.setState({ loading: true });
		const path = `/file/${this.state.path.join('')}${file.name}`;
		await fetch(path, {
			method: 'PUT',
			body: file,
		}).then(async res => {
			console.debug(res);
			this.loadFiles();
		}).catch(reason => {
			console.error(reason);
		}).finally(() => {
			this.setState({ loading: false });
		});
	};

	downloadFile = async (name: string) => {
		this.setState({ loading: true });
		const path = `/file/${this.state.path.join('')}${name}`;
		await fetch(path, {
			method: 'GET',
		}).then(async res => {
			console.debug(res);
			const blob = await res.blob();
			const url = URL.createObjectURL(blob);

			const a = document.createElement('a');
			a.href = url;
			a.download = name;
			a.click();

			URL.revokeObjectURL(url);
		}).catch(reason => {
			console.error(reason);
		}).finally(() => {
			this.setState({ loading: false });
		});
	};

	deleteFile = async (name: string) => {
		this.setState({ loading: true });
		const path = `/file/${this.state.path.join('')}${name}`;
		await fetch(path, {
			method: 'DELETE',
		}).then(async res => {
			console.debug(res);
			this.loadFiles();
		}).catch(reason => {
			console.error(reason);
		}).finally(() => {
			this.setState({ loading: false });
		});
	};

	componentDidUpdate(previousProps: Readonly<{}>, previousState: Readonly<FileBrowserState>): void {
		if (previousState.path !== this.state.path) {
			this.onPathChanged(previousState.path);
		}
	}

	async onPathChanged(prevPath: string[]) {
		if (!await this.loadFiles()) {
			this.setState({ path: prevPath });
			return;
		}
		this.setState({ pathInput: `/${this.state.path.join('')}` });
	}

	// Lifecycle: Called whenever our component is created
	componentDidMount() {
		this.loadFiles();
	}

	// Lifecycle: Called just before our component will be destroyed
	componentWillUnmount() {

	}

	goUp = () => {
		this.setState({ path: this.state.path.slice(0, -1) });
	};

	goInto = (subfolder: string) => {
		this.setState({ path: [...this.state.path, subfolder] });
	};

	render() {
		return (
			<div class='file-browser'>
				<Spinner loading={this.state.loading}>
					<div class='toolbar'>
						<RotateCcw class="actions" onClick={this.loadFiles} aria-label="Refresh" />
						<input
							value={this.state.pathInput}
							onInput={(e) => this.setState({ pathInput: (e.target as HTMLInputElement).value })}
							onKeyDown={(e) => {
								if (e.key === 'Escape') {
									this.setState({ pathInput: `/${this.state.path.join('')}` });
								}
								else if (e.key === 'Enter') {
									const parts = this.state.pathInput.split('/').filter(Boolean).map(value => `${value}/`);
									this.setState({ path: parts });
								}
							}} />
						<input
							type="file"
							accept='.json'
							style={{ display: 'none' }}
							ref={(el) => { (this.fileInput = el!); }}
							onChange={(e) => {
								const file = (e.target as HTMLInputElement).files?.[0];
								if (file) this.uploadFile(file);
							}}
						/>

						<FilePlusCorner class="actions"
							onClick={() => {
								if (this.fileInput) {
									this.fileInput.value = '';
									this.fileInput.click();
								}
							}}
						/>
					</div>
					<table>
						<tbody>
							{this.state.path.length > 0 && (
								<tr>
									<td class='icon-cell folder' >
										<Folder />
									</td>
									<td class="name-cell folder" onClick={this.goUp}>
										<span>..</span>
									</td>
								</tr>
							)}
							{this.state.entries.length <= 0 && (
								<tr>
									<td>There are no entries for this directory</td>
								</tr>
							)}

							{this.state.entries
								.filter((value) => value.endsWith('/'))
								.map((name) => (
									<tr key={name}>
										<td class='icon-cell folder' >
											<Folder />
										</td>
										<td class="name-cell folder" onClick={() => this.goInto(name)}>
											<span>{name}</span>
										</td>
									</tr>
								))}

							{this.state.entries
								.filter((value) => !value.endsWith('/'))
								.map((name) => (
									<tr key={name}>
										<td class='icon-cell file' >
											<File />
										</td>
										<td class="name-cell file">
											<span>{name}</span>
										</td>
										<td class='actions-cell'>
											<ArrowDownToLine class="actions" onClick={() => this.downloadFile(name)} aria-label="Download file" />
											<Trash2 class="actions" onClick={() => this.deleteFile(name)} aria-label="Delete file" />
										</td>
									</tr>
								))}
						</tbody>
					</table>
				</Spinner>
			</div >
		);
	}
}
