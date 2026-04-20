# Endpoints

| Protocol | URI | Method | Description |
| --- | --- | --- | --- |
| http | `/*` | GET | Redirect to https |
| wss | `/ws` | GET | Connection to WebSocket server |
| https | `/assets/app.js` | GET | The `app.js` file used by the `index.html`  |
| https | `/assets/index.css` | GET | The `index.css` file used by the `index.html` |
| https | `/assets/worldmap.svg` | GET | The `worldmap.svg` file used by the `index.html` |
| https | `/config` | GET | Rest API endpoint which accepts query parameter `/config?key` to request the value |
| https | `/config` | PUT | Rest API endpoint which accepts query parameter `/config?key=value` to update the value |
| https | `/config` | DELETE | Rest API endpoint which accepts query parameter `/config?key` to rest the value to default |
| https | `/directory` | GET | For browsing the available files. Will return the files/folder listed in the filesystem at root position |
| https | `/directory/<directory path>` | GET | For browsing the available files. Will return the files/folder listed in the filesystem at `<directory path>` position |
| https | `/file/<file path>` | DELETE | Command for deleting a file in the filesystem on the SD Card |
| https | `/file/<file path>` | GET | For downloading files stored in the filesystem on the SD Card |
| https | `/file/<file path>` | PUT | For uploading files into the filesystem on the SD Card. Files are sent as body of HTTP post requests |
| https | `/vite.svg` | GET | The `vite.svg` file used by the `index.html` |
| https | `/*` | GET | Serves as endpoint for everything else and responds with the `index.html` which than handles the routing in the browser <br> - `/` Home <br> - `/about` About <br> - `/simulation` Simulation preview <br> - `/browser` SD Card File Browser <br> - `*` Not Found page  |

# mDNS 
The ESP does with mDNS self advertises its hostname (CONFIG_LWIP_LOCAL_HOSTNAME) in the local network and is accessible via hostname.local for the others in the same network. 