/**
 * Smart Parking Dashboard — Frontend JS
 * Polls /api/status every 1s and /api/logs every 2s
 * All updates are animated and non-blocking
 */

const API = '';  // empty = same origin (localhost:8080)
let state = { slots: [], rows: 0, cols: 0, total: 0, used: 0, avail: 0 };
let prevState = null;
let flashedSlots = new Set();
let selectedGate = 0;
let logCount = 0;

// ============================================================
// Init
// ============================================================
document.addEventListener('DOMContentLoaded', () => {
    startClock();
    fetchStatus();
    fetchLogs();
    setInterval(fetchStatus, 1200);
    setInterval(fetchLogs,   2000);
    setInterval(startClock,  1000);
});

function startClock() {
    const el = document.getElementById('clock');
    if (!el) return;
    const now = new Date();
    el.textContent = now.toLocaleTimeString('en-GB', { hour12: false });
}

// ============================================================
// Data fetching
// ============================================================
async function fetchStatus() {
    try {
        const res = await fetch(API + '/api/status');
        if (!res.ok) return;
        const data = await res.json();
        prevState = state;
        state = data;
        renderGrid();
        renderStats();
    } catch (e) {
        // Server not connected yet — show disconnected state gracefully
        document.getElementById('live-dot')?.classList.add('offline');
    }
}

async function fetchLogs() {
    try {
        const res = await fetch(API + '/api/logs');
        if (!res.ok) return;
        const logs = await res.json();
        renderLogs(logs);
    } catch (e) { /* silent */ }
}

// ============================================================
// Render: Stats cards
// ============================================================
function renderStats() {
    animateNumber('stat-total', state.total);
    animateNumber('stat-used',  state.used);
    animateNumber('stat-avail', state.avail);

    const pct = state.total > 0 ? Math.round(state.used / state.total * 100) : 0;
    animateNumber('stat-pct', pct, '%');

    // Occupancy bar
    const fill = document.getElementById('occ-fill');
    const pctLabel = document.getElementById('occ-pct');
    if (fill) fill.style.width = pct + '%';
    if (pctLabel) {
        pctLabel.textContent = pct + '%';
        pctLabel.style.color = pct > 80 ? 'var(--red)' : pct > 50 ? 'var(--yellow)' : 'var(--cyan)';
    }
}

function animateNumber(id, target, suffix = '') {
    const el = document.getElementById(id);
    if (!el) return;
    const current = parseInt(el.textContent) || 0;
    if (current === target) return;

    // Quick count animation
    const diff  = target - current;
    const steps = Math.min(Math.abs(diff), 8);
    let  step   = 0;

    const interval = setInterval(() => {
        step++;
        const val = Math.round(current + diff * (step / steps));
        el.textContent = val + suffix;
        if (step >= steps) clearInterval(interval);
    }, 30);
}

// ============================================================
// Render: Parking grid
// ============================================================
function renderGrid() {
    const container = document.getElementById('parking-grid');
    if (!container || !state.slots || state.slots.length === 0) return;

    const cols = state.cols;
    container.style.gridTemplateColumns = `repeat(${cols}, 1fr)`;

    // Detect changed slots for flash animation
    const changedSlots = new Set();
    if (prevState && prevState.slots) {
        prevState.slots.forEach(ps => {
            const cur = state.slots.find(s => s.id === ps.id);
            if (cur && cur.occupied !== ps.occupied) {
                changedSlots.add(cur.id);
            }
        });
    }

    // Build or update DOM nodes
    state.slots.forEach(slot => {
        let el = document.getElementById('slot-' + slot.id);

        if (!el) {
            el = document.createElement('div');
            el.id = 'slot-' + slot.id;
            el.className = 'slot';
            el.innerHTML = `
                <span class="slot-id">#${slot.id}</span>
                <span class="slot-icon"></span>
                <span class="slot-plate"></span>
            `;
            el.addEventListener('mouseenter', (e) => showSlotPopup(e, slot));
            el.addEventListener('mouseleave',  hideSlotPopup);
            el.addEventListener('mousemove',   (e) => moveSlotPopup(e));
            el.addEventListener('click',       () => onSlotClick(slot));
            container.appendChild(el);
        }

        // Update state classes
        el.classList.remove('free', 'occupied', 'flash');
        el.classList.add(slot.occupied ? 'occupied' : 'free');

        // Flash on change
        if (changedSlots.has(slot.id)) {
            void el.offsetWidth; // force reflow to restart animation
            el.classList.add('flash');
            setTimeout(() => el.classList.remove('flash'), 900);
        }

        // Update content
        el.querySelector('.slot-icon').textContent = slot.occupied ? '🚗' : '';
        const plateEl = el.querySelector('.slot-plate');
        plateEl.textContent = slot.occupied ? (slot.plate || '') : '';

        // Update tooltip data
        el._slotData = slot;
        el.onmouseenter = (e) => showSlotPopup(e, slot);
    });

    // Gate markers (position absolutely relative to grid)
    renderGateMarkers();
}

function renderGateMarkers() {
    // Gates are just styled labels on the corner slots
    if (!state.slots || state.slots.length === 0) return;
    const rows = state.rows, cols = state.cols;
    const gateSlots = [
        { slotId: 0,                      label: '⬆ G0' },
        { slotId: cols - 1,               label: '⬆ G1' },
        { slotId: (rows-1)*cols,          label: '⬇ G2' },
        { slotId: rows * cols - 1,        label: '⬇ G3' },
    ];
    gateSlots.forEach(g => {
        const el = document.getElementById('slot-' + g.slotId);
        if (!el) return;
        let badge = el.querySelector('.gate-badge');
        if (!badge) {
            badge = document.createElement('span');
            badge.className = 'gate-badge';
            badge.style.cssText = `
                position:absolute; top:-18px; left:50%; transform:translateX(-50%);
                font-size:8px; color:var(--yellow); font-family:var(--font-display);
                letter-spacing:1px; white-space:nowrap; text-shadow:0 0 6px #ffe600;
            `;
            el.style.overflow = 'visible';
            el.appendChild(badge);
        }
        badge.textContent = g.label;
    });
}

// ============================================================
// Render: Logs
// ============================================================
function renderLogs(logs) {
    const list = document.getElementById('log-list');
    if (!list) return;

    // Only append new entries (track by count)
    if (logs.length === logCount) return;

    const newLogs = logs.slice(logCount);
    logCount = logs.length;

    newLogs.forEach(log => {
        const item = document.createElement('div');
        item.className = `log-item ${log.type}`;
        item.innerHTML = `
            <span class="log-time">${log.time}</span>
            <span class="log-msg">${escapeHtml(log.msg)}</span>
        `;
        list.appendChild(item);
    });

    // Keep max 80 entries in DOM
    while (list.children.length > 80) {
        list.removeChild(list.firstChild);
    }

    // Auto-scroll to bottom
    list.scrollTop = list.scrollHeight;
}

// ============================================================
// Controls
// ============================================================
function selectGate(n) {
    selectedGate = n;
    document.querySelectorAll('.gate-btn').forEach((btn, i) => {
        btn.classList.toggle('active', i === n);
    });
}

async function triggerEntry() {
    try {
        const res  = await fetch(API + '/api/entry', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ gate: selectedGate })
        });
        const data = await res.json();
        if (data.error === 'lot_full') {
            showToast('⚠ LOT FULL — cannot enter');
        } else if (data.plate) {
            showToast(`✓ Entry: ${data.plate} → Slot #${data.slot}`);
            fetchStatus();
            fetchLogs();
        }
    } catch (e) { showToast('⚠ Server offline'); }
}

async function triggerExit() {
    try {
        const res  = await fetch(API + '/api/exit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({})
        });
        const data = await res.json();
        if (data.error) {
            showToast('⚠ No occupied slots to exit');
        } else {
            showToast(`✓ Exit: ${data.plate} ← Slot #${data.slot}`);
            fetchStatus();
            fetchLogs();
        }
    } catch (e) { showToast('⚠ Server offline'); }
}

async function simulateBurst() {
    showToast('⚡ Simulating 5 entries...');
    for (let i = 0; i < 5; i++) {
        setTimeout(() => triggerEntry(), i * 350);
    }
}

async function clearAll() {
    // Exit all occupied slots
    const occupied = state.slots.filter(s => s.occupied);
    if (occupied.length === 0) { showToast('Lot is already empty'); return; }
    showToast(`Clearing ${occupied.length} vehicles...`);
    for (let i = 0; i < occupied.length; i++) {
        setTimeout(async () => {
            await fetch(API + '/api/exit', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ plate: occupied[i].plate })
            });
            fetchStatus();
        }, i * 150);
    }
}

// ============================================================
// Slot click (trigger exit for occupied)
// ============================================================
function onSlotClick(slot) {
    if (!slot.occupied) {
        // Trigger entry at gate 0 (quick action)
        triggerEntry();
        return;
    }
    // Exit this specific vehicle
    fetch(API + '/api/exit', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ plate: slot.plate })
    }).then(() => { fetchStatus(); fetchLogs(); });
    showToast(`↩ ${slot.plate} exiting Slot #${slot.id}`);
}

// ============================================================
// Slot popup
// ============================================================
let popup = null;

function showSlotPopup(e, slot) {
    if (!popup) {
        popup = document.createElement('div');
        popup.className = 'slot-popup';
        document.body.appendChild(popup);
    }
    popup.innerHTML = `
        <div class="slot-popup-title">SLOT #${slot.id}</div>
        <div class="slot-popup-row"><span>Status</span><span style="color:${slot.occupied?'var(--red)':'var(--green)'}">
            ${slot.occupied ? 'OCCUPIED' : 'FREE'}</span></div>
        ${slot.occupied ? `
        <div class="slot-popup-row"><span>Plate</span><span>${slot.plate}</span></div>
        <div class="slot-popup-row"><span>Row</span><span>${slot.row}</span></div>
        <div class="slot-popup-row"><span>Col</span><span>${slot.col}</span></div>
        <div class="slot-popup-row" style="margin-top:4px;font-size:9px;color:var(--muted)">
            <span>Click to exit</span>
        </div>` : `
        <div class="slot-popup-row" style="font-size:9px;color:var(--muted)">
            <span>Click to enter</span>
        </div>`}
    `;
    popup.style.display = 'block';
    moveSlotPopup(e);
}

function moveSlotPopup(e) {
    if (!popup) return;
    popup.style.left = (e.clientX + 14) + 'px';
    popup.style.top  = (e.clientY - 10) + 'px';
}

function hideSlotPopup() {
    if (popup) popup.style.display = 'none';
}

// ============================================================
// Toast
// ============================================================
let toastTimer = null;

function showToast(msg) {
    const el = document.getElementById('toast');
    if (!el) return;
    el.textContent = msg;
    el.classList.add('show');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => el.classList.remove('show'), 2800);
}

// ============================================================
// Helpers
// ============================================================
function escapeHtml(str) {
    return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
