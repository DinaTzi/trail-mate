# Map Tile Source / Cache Runtime Spec

## Rule

Tile lookup and cache ownership belong to map runtime/source adapters.

Renderers draw tiles. They do not own Trail Mate tile storage paths.
They also do not own tile URL policy, network download policy, retry policy, or
contour data-source credentials.

`MapWorkspaceSnapshot` carries viewport, layer, and tool state. It must not carry tile bitmap bytes, decoded image cache objects, or filesystem cache objects.

Tile source/cache work must also conform to
`UI_STORAGE_EVENT_RUNTIME_DESIGN_SPEC.md`: UI owner code submits tile intents,
workers perform filesystem/storage work, and tile results return as events. A
renderer must not open tile files, wait for shared-SPI, or decode tile payloads
from an input callback, LVGL timer, GTK callback, or page render callback.

## Objects

| Object | Pattern | Responsibility | Forbidden |
| --- | --- | --- | --- |
| `MapTileRef` | Value Object | Identify layer/z/x/y | Filesystem path, LVGL object |
| `MapTileLayer` | Layer key | Identify OSM/Terrain/Satellite/Contour layers | Renderer details |
| `MapTileFormat` | Format enum | Identify PNG/JPEG payload format | Decode ownership |
| `MapTilePayload` | Data DTO | Optional byte payload returned by a source | Filesystem path ownership |
| `IMapTileSource` | Port | Lookup/read tile payload by `MapTileRef` | LVGL rendering |
| `IMapTileFileSystem` | Port | Filesystem operations needed by filesystem source | Tile path policy |
| `IMapTileCache` | Repository / Cache port | Runtime cache/fill contract | UI rendering |
| `MapTileResolver` | Strategy | Resolve `MapTileRef` to Trail Mate storage path | LVGL, filesystem reads |
| `FilesystemMapTileSource` | Source adapter | Wrap filesystem tile lookup behind `IMapTileSource` | Spread path mapping back to renderer |

## Directory Mapping

`MapTileResolver` maps:

- `MapTileLayer::Osm` -> `/maps/base/osm/{z}/{x}/{y}.png`
- `MapTileLayer::Terrain` -> `/maps/base/terrain/{z}/{x}/{y}.png`
- `MapTileLayer::Satellite` -> `/maps/base/satellite/{z}/{x}/{y}.jpg`
- `MapTileLayer::ContourMajor500` -> `/maps/contour/major-500/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMajor200` -> `/maps/contour/major-200/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMajor100` -> `/maps/contour/major-100/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMajor50` -> `/maps/contour/major-50/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMajor25` -> `/maps/contour/major-25/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMinor100` -> `/maps/contour/minor-100/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMinor50` -> `/maps/contour/minor-50/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMinor20` -> `/maps/contour/minor-20/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMinor10` -> `/maps/contour/minor-10/{z}/{x}/{y}.png`
- `MapTileLayer::ContourMinor5` -> `/maps/contour/minor-5/{z}/{x}/{y}.png`

The resolver may prepend a target root prefix such as `A:` or a Linux SD root.

## Platform Behavior

All targets converge on the same observable tile result: a `MapTileRef` resolves
to the Trail Mate cache layout above and, when available, renders as a base or
contour image. Targets may reach that result through different runtime
capabilities:

| Runtime | Missing base tile behavior | Missing contour behavior | Forbidden |
| --- | --- | --- | --- |
| ESP32 / embedded SD runtimes | Offline only. Check the SD/cache layout and report missing data. | Offline only. Check existing contour tiles and report missing data. | Network tile download, URL policy in UI pages |
| Linux compact LVGL runtimes, including Cardputer Zero | Online-capable. Queue missing OSM/Terrain/Satellite tiles through Linux `MapTileCache`; cache successful downloads back into `maps/base/...`. | Use cached contour tiles. When Earthdata credentials and the Linux contour toolchain are available, queue generation through `MapContourTileGenerator`; otherwise keep the contour layer missing. | Page-owned curl/download code, macro-gated page behavior, simulator-specific cache paths |
| uConsole GTK runtime | Online-capable through its map workspace model and the same Linux `MapTileCache`. | Generates through `MapContourTileGenerator` when credentials/tooling are configured. | A separate cache layout or page-owned URL policy |

Linux base-layer URL templates remain Linux runtime configuration:

- `TRAIL_MATE_OSM_TILE_URL`
- `TRAIL_MATE_TERRAIN_TILE_URL`
- `TRAIL_MATE_SATELLITE_TILE_URL`

Linux HTTP resolver behavior remains Linux runtime configuration:

- `TRAIL_MATE_MAP_DOH_URL` / `TRAIL_MATE_CURL_DOH_URL`: optional DoH endpoint;
  unset means use the system resolver.
- `TRAIL_MATE_MAP_IP_RESOLVE` / `TRAIL_MATE_CURL_IP_RESOLVE`: optional `ipv4`
  or `ipv6` libcurl IP-family selection.

Linux contour generation credentials are runtime configuration:

- `TRAIL_MATE_EARTHDATA_TOKEN`
- `TRAIL_MATE_EARTH_DATA_TOKEN`
- persisted Linux map setting `uconsole_map/earthdata_token`

The page layer may show missing/loading/ready states, but it must not decide how
to fetch, authenticate, cache, or retry tiles.

## Phase 7.10 First Slice

Phase 7.10 introduces:

- `MapTileRef`
- `MapTileLayer`
- `MapTileFormat`
- `MapTileStatus`
- `MapTilePayload`
- `MapTileLookupResult`
- `IMapTileSource`
- `IMapTileCache`
- `MapTileResolver`
- `FilesystemMapTileSource`

Existing platform map tile runtimes may keep LVGL drawing and decoded image cache logic temporarily, but path mapping must flow through `FilesystemMapTileSource`.

## Non-Goals

Phase 7.10 does not:

- implement online tile download for ESP32 / embedded SD runtimes
- change Trail Mate offline tile directory structure
- store tile bitmaps in `MapWorkspaceSnapshot`
- implement a full LRU cache
- redesign Map overlay
- move route/tracker ownership
- generate contour tiles
- change Trail Mate Center
- change UX Pack
- change repository layout
