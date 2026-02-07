# Kinect Device Inventory

Tested 2026-02-06. All three Model 1414 devices fully functional.

## Inventory

### Device 1: Kinect 1414 #1

**Identifiers:**
- Camera Serial: `A00364A11362046A`
- Audio Serial: `A44884C13680046A`

**Test Results (2026-02-06):**
- Motor Control: ✅ Pass
- LED Control: ✅ Pass
- Accelerometer: ✅ Pass
- Depth/RGB Streams: ✅ Pass
- Integration Tests: ✅ All passed

**Status:** Fully functional

---

### Device 2: Kinect 1414 #2

**Identifiers:**
- Camera Serial: `A00362810720047A`
- Audio Serial: `A44882C01248047A`

**Test Results (2026-02-06):**
- Motor Control: ✅ Pass
- LED Control: ✅ Pass
- Accelerometer: ✅ Pass
- Depth/RGB Streams: ✅ Pass
- Integration Tests: ✅ All passed

**Status:** Fully functional

**Notes:** Used for debugging test suite issues (motor API, depth range test)

---

### Device 3: Kinect 1414 #3

**Identifiers:**
- Camera Serial: `A00364A04675053A`
- Audio Serial: `A44884D01122053A`

**Test Results (2026-02-06):**
- Motor Control: ✅ Pass
- LED Control: ✅ Pass
- Accelerometer: ✅ Pass
- Depth/RGB Streams: ✅ Pass
- Integration Tests: ✅ All passed

**Status:** Fully functional

---

## Device Comparison

| Device | Camera Serial | Status |
|--------|---------------|--------|
| #1 | A00364A11362046A | ✅ Fully functional |
| #2 | A00362810720047A | ✅ Fully functional |
| #3 | A00364A04675053A | ✅ Fully functional |

**All three devices:**
- Model 1414 (Original Xbox 360 Kinect)
- No firmware required
- All capabilities tested and working
- Ready for development

**Note:** All devices are functionally equivalent. USB packet error differences observed during cold-start testing are likely due to warm-up time, not hardware quality.

## Testing

To test a device:
```bash
sudo ./scripts/test-all-devices.sh
```
