# ExamplesLandingPage

**Status:** complete
**Created:** 2026-02-06
**Branch:** feature/013-ExamplesLandingPage

## Summary

Create a landing page for Kinect XR web examples at `localhost:3000` root. Currently the root serves `test.html` (RGB+Depth viewer) directly. This spec adds a clean examples gallery with cards linking to each demo, making it easier to discover and navigate between examples.

The landing page will be static HTML/CSS with no build step, consistent with the existing `web/` directory patterns. The current RGB+Depth viewer will move to `/examples/rgb-depth/` and the Three.js and P5.js examples already exist at `/examples/threejs/` and `/examples/p5js/`.

## Scope

**In:**
- Create `web/index.html` as the new landing page
- Move current `web/test.html` content to `web/examples/rgb-depth/index.html`
- Create directory structure for the RGB+Depth example
- Update `web/serve.js` to serve `index.html` at root instead of `test.html`
- Responsive card grid (3 columns desktop, 1 column mobile)
- Placeholder images for example thumbnails (user will add real screenshots later)
- Cards for: Three.js Point Cloud, RGB+Depth Viewer, P5.js (existing examples)

**Out:**
- Actual screenshots/thumbnails (placeholders only)
- Pagination (single page fits 3-12 examples)
- JavaScript interactivity on landing page
- Additional examples beyond existing three

## Acceptance Criteria

### Descriptive Criteria

- [x] Landing page loads at `localhost:3000/`
- [x] All three example cards are visible and clickable
- [x] Each card links to correct example path
- [x] Layout is responsive (3 columns on desktop, 1 on mobile)
- [x] Existing examples continue to work at their paths
- [x] No build step required (static HTML/CSS)

### Executable Tests (for UI-observable behavior)

#### AC1: Landing page title and header visible
```typescript
await page.goto('http://localhost:3000/');
const title = page.locator('h1');
await expect(title).toBeVisible();
await expect(title).toHaveText(/Kinect XR/i);
```

#### AC2: Three example cards are present
```typescript
await page.goto('http://localhost:3000/');
const cards = page.locator('[data-testid="example-card"]');
await expect(cards).toHaveCount(3);
```

#### AC3: Three.js example card links correctly
```typescript
await page.goto('http://localhost:3000/');
const threejsLink = page.locator('a[href*="threejs"]');
await expect(threejsLink).toBeVisible();
await threejsLink.click();
await expect(page).toHaveURL(/\/examples\/threejs/);
```

#### AC4: RGB+Depth example accessible at new path
```typescript
await page.goto('http://localhost:3000/examples/rgb-depth/');
const title = page.locator('h1');
await expect(title).toHaveText(/Kinect XR/i);
```

#### AC5: Responsive layout (mobile view shows single column)
```typescript
await page.setViewportSize({ width: 375, height: 667 });
await page.goto('http://localhost:3000/');
const cards = page.locator('[data-testid="example-card"]');
// Verify cards stack vertically by checking first two cards have same X position
const box1 = await cards.nth(0).boundingBox();
const box2 = await cards.nth(1).boundingBox();
expect(box1.x).toBe(box2.x); // Same horizontal position = stacked
```

## Architecture Delta

**Before:**
- `web/serve.js` redirects `/` to `/test.html`
- `web/test.html` is the RGB+Depth viewer
- `web/examples/` contains `threejs/` and `p5js/` examples
- No landing page or example gallery

**After:**
- `web/serve.js` redirects `/` to `/index.html`
- `web/index.html` is the examples landing page with card grid
- `web/examples/rgb-depth/index.html` contains the RGB+Depth viewer (moved from test.html)
- `web/test.html` removed (content moved)
- All examples accessible from gallery cards

## Milestones

- [x] M1: Create `web/examples/rgb-depth/` directory and move `test.html` content there (update lib import paths)
- [x] M2: Create `web/index.html` landing page with responsive card grid layout
- [x] M3: Add cards for Three.js, RGB+Depth, and P5.js examples with placeholder images
- [x] M4: Update `web/serve.js` to serve `index.html` at root
- [x] M5: Verify all examples work at their respective paths

## Open Questions

None - requirements are clear.

---

## Implementation Log

### Milestone 1
- Created `web/examples/rgb-depth/` directory
- Moved `test.html` content to `web/examples/rgb-depth/index.html`
- Updated import path to `../../lib/kinect-client.js`
- Added "Back to Examples" link

### Milestone 2-3
- Created `web/index.html` with dark theme matching existing examples
- CSS Grid layout: 1 column mobile, 2 columns tablet, 3 columns desktop
- Used breakpoints at 640px and 960px
- Cards have hover effects and SVG icon placeholders

### Milestone 4
- Updated `web/serve.js` to serve `/index.html` at root instead of `/test.html`
- Removed `test.html` (content preserved in rgb-depth example)

### Milestone 5
- Verified file structure: all HTML files in correct locations
- Verified import paths: all examples use `../../lib/kinect-client.js`
- Verified 3 cards with `data-testid="example-card"` present

---

## Documentation Updates

### web/README.md Updates
- Updated directory structure to show new layout
- Updated examples section to reference landing page
- Added Three.js and RGB+Depth viewer descriptions
- Removed reference to deleted `test.html`

---

## Archive Criteria

**Complete these BEFORE moving spec to archive:**

- [x] All milestones complete
- [x] All acceptance criteria met
- [x] Tests passing (if applicable)
- [x] **Proposed doc updates drafted** in section above (based on actual implementation)
- [x] **PRD.md updated** if features, use cases, or product strategy changed (N/A - web examples only, no core product change)
- [x] **ARCHITECTURE.md updated** if technical architecture or system design changed (N/A - static HTML, no architecture change)
- [ ] Spec moved to `specs/archive/NNN-SpecName.md`
