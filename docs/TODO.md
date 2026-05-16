# Typespeed — TODO / Roadmap

Items are roughly ordered by dependency. Check them off as they land.

---

## UI Redesign

- [ ] **Complete redesign of all menus and stats screens using HTML-based design language**
  - Replace the current Allegro-drawn buttons and text with a visual style inspired by modern web UI (clean typography, consistent spacing, card-based layouts, subtle shadows, proper hierarchy)
  - Applies to: Menu, Category, File Pick, Preview, Playing toolbar, Results, Global Stats
  - Reference: look at Monkeytype, typing.io for inspiration

---

## Architecture Migration

- [ ] **Move the project frontend to HTML/CSS/JS, keep the backend in C++**
  - The game engine (input timing, WPM calculation, passage progression, key logging) stays in C++ for accuracy and speed — this is the critical path where milliseconds matter
  - The UI layer (menus, stats display, heatmap rendering, results screen) moves to a browser-based frontend
  - Communication between frontend and backend: most likely a local WebSocket or HTTP server embedded in the C++ binary (consider `cpp-httplib` or a lightweight Boost.Beast server), or alternatively compile the game logic to WebAssembly (Emscripten) and call it from JS
  - Key decisions to make before starting:
    - [ ] WebSocket/HTTP local server vs WebAssembly?
    - [ ] Single-page app (Vite + vanilla JS) vs framework (React/Svelte)?
    - [ ] How to handle the Allegro event loop alongside a network server (separate thread? replace Allegro entirely for input?)
  - The Allegro dependency can be dropped for everything except the keyboard event source once the frontend is HTML — or replaced with raw X11/evdev input if going headless

---

## Smaller pending items

- [ ] Add more sample text files to each category
- [ ] Bigram analysis on global stats screen (slowest two-key sequences from keylog)
- [ ] Sessions-per-day heatmap (GitHub-style calendar) on global stats screen
- [ ] Personal bests per category on global stats screen
- [ ] WPM goal tracker + mastery badges (Bronze/Silver/Gold per category)
- [ ] Nemesis key highlight (the single key dragging your WPM down most)
- [ ] Results screen: show moment-to-moment WPM graph (samples already collected in history.txt)
