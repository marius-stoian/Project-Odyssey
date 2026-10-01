using System;
using System.Collections.Generic;

public class Sheet {
    public int W, H;
    public byte[] Px; // BGRA
    public Sheet(int w, int h, byte[] px) { W = w; H = h; Px = px; }
    public int R(int x, int y) { return Px[(y * W + x) * 4 + 2]; }
    public int G(int x, int y) { return Px[(y * W + x) * 4 + 1]; }
    public int B(int x, int y) { return Px[(y * W + x) * 4 + 0]; }
    public int Max(int x, int y) { return Math.Max(R(x, y), Math.Max(G(x, y), B(x, y))); }

    // Connected boxes of pixels whose brightest channel is <= dark and whose channels are close
    // (grey, not coloured shadow); keeps boxes of the given size range and fill ratio.
    public List<int[]> DarkBoxes(int dark, int minW, int maxW, int minH, int maxH, double minFill) {
        var seen = new bool[W * H];
        var list = new List<int[]>();
        var stack = new Stack<int>();
        for (int y0 = 0; y0 < H; y0++) for (int x0 = 0; x0 < W; x0++) {
            int i0 = y0 * W + x0;
            if (seen[i0] || !IsDark(x0, y0, dark)) continue;
            int x1 = x0, x2 = x0, y1 = y0, y2 = y0, n = 0;
            seen[i0] = true; stack.Push(i0);
            while (stack.Count > 0) {
                int i = stack.Pop(); int x = i % W, y = i / W; n++;
                if (x < x1) x1 = x; if (x > x2) x2 = x; if (y < y1) y1 = y; if (y > y2) y2 = y;
                if (x > 0) Try(x - 1, y, dark, seen, stack);
                if (x < W - 1) Try(x + 1, y, dark, seen, stack);
                if (y > 0) Try(x, y - 1, dark, seen, stack);
                if (y < H - 1) Try(x, y + 1, dark, seen, stack);
            }
            int w = x2 - x1 + 1, h = y2 - y1 + 1;
            if (w >= minW && w <= maxW && h >= minH && h <= maxH && n >= minFill * w * h) list.Add(new[] { x1, y1, w, h });
        }
        return list;
    }
    bool IsDark(int x, int y, int dark) {
        int r = R(x, y), g = G(x, y), b = B(x, y);
        int mx = Math.Max(r, Math.Max(g, b)), mn = Math.Min(r, Math.Min(g, b));
        return mx <= dark && mx - mn <= 12;
    }
    void Try(int x, int y, int dark, bool[] seen, Stack<int> stack) {
        int i = y * W + x;
        if (!seen[i] && IsDark(x, y, dark)) { seen[i] = true; stack.Push(i); }
    }
    // Mean brightness of each column / row inside a rectangle: to find grid lines.
    public double[] ColumnMean(int x, int y, int w, int h) {
        var m = new double[w];
        for (int i = 0; i < w; i++) { double s = 0; for (int j = 0; j < h; j++) s += Max(x + i, y + j); m[i] = s / h; }
        return m;
    }
    public double[] RowMean(int x, int y, int w, int h) {
        var m = new double[h];
        for (int j = 0; j < h; j++) { double s = 0; for (int i = 0; i < w; i++) s += Max(x + i, y + j); m[j] = s / w; }
        return m;
    }
    // Per column (axis 0) or row (axis 1): how many pixels are close to colour (r, g, b).
    public int[] Count(int axis, int r, int g, int b, int tol) {
        var n = new int[axis == 0 ? W : H];
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            if (Math.Abs(R(x, y) - r) + Math.Abs(G(x, y) - g) + Math.Abs(B(x, y) - b) <= tol) n[axis == 0 ? x : y]++;
        }
        return n;
    }
    public string Peaks(int axis, int r, int g, int b, int tol, int min) {
        var n = Count(axis, r, g, b, tol); var s = new System.Text.StringBuilder(); int start = -1;
        for (int i = 0; i <= n.Length; i++) {
            bool on = i < n.Length && n[i] >= min;
            if (on && start < 0) start = i;
            if (!on && start >= 0) { s.Append(start).Append('-').Append(i - 1).Append(' '); start = -1; }
        }
        return s.ToString();
    }
    public string Pixel(int x, int y) { return R(x, y) + "," + G(x, y) + "," + B(x, y); }
    // Items on a dark grid: pixels brighter than `dark` (brightest channel) are item pixels. Blobs
    // (8-connected) of at least minSize pixels are gathered into the cell of their centre, on a
    // cols x rows grid over the area; returns one "x,y,w,h" per cell, reading order, "" if empty.
    public string[] GridItems(int ax, int ay, int aw, int ah, int cols, int rows, int dark, int minSize, int pad) {
        var seen = new bool[W * H]; var stack = new Stack<int>();
        var box = new int[cols * rows][];
        for (int y0 = ay; y0 < ay + ah; y0++) for (int x0 = ax; x0 < ax + aw; x0++) {
            int i0 = y0 * W + x0; if (seen[i0] || Max(x0, y0) <= dark) continue;
            int x1 = x0, x2 = x0, y1 = y0, y2 = y0, n = 0; seen[i0] = true; stack.Push(i0);
            while (stack.Count > 0) {
                int i = stack.Pop(); int x = i % W, y = i / W; n++;
                if (x < x1) x1 = x; if (x > x2) x2 = x; if (y < y1) y1 = y; if (y > y2) y2 = y;
                for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < ax || ny < ay || nx >= ax + aw || ny >= ay + ah) continue;
                    int j = ny * W + nx; if (!seen[j] && Max(nx, ny) > dark) { seen[j] = true; stack.Push(j); }
                }
            }
            if (n < minSize) continue;
            int cx = Math.Min(cols - 1, ((x1 + x2) / 2 - ax) * cols / aw), cy = Math.Min(rows - 1, ((y1 + y2) / 2 - ay) * rows / ah);
            int c = cy * cols + cx;
            if (box[c] == null) box[c] = new[] { x1, y1, x2, y2 };
            else { box[c][0] = Math.Min(box[c][0], x1); box[c][1] = Math.Min(box[c][1], y1); box[c][2] = Math.Max(box[c][2], x2); box[c][3] = Math.Max(box[c][3], y2); }
        }
        var outp = new string[cols * rows];
        for (int c = 0; c < box.Length; c++) {
            if (box[c] == null) { outp[c] = ""; continue; }
            int x1 = Math.Max(ax, box[c][0] - pad), y1 = Math.Max(ay, box[c][1] - pad), x2 = Math.Min(ax + aw - 1, box[c][2] + pad), y2 = Math.Min(ay + ah - 1, box[c][3] + pad);
            outp[c] = x1 + "," + y1 + "," + (x2 - x1 + 1) + "," + (y2 - y1 + 1);
        }
        return outp;
    }
    // Thin light lines: per column (axis 0) or row (axis 1), how many pixels are brighter by
    // `step` than the pixels two away on both sides across the line, and not bright themselves.
    public string Ridges(int axis, int step, int maxBright, int min) {
        int len = axis == 0 ? W : H; var n = new int[len];
        for (int y = 2; y < H - 2; y++) for (int x = 2; x < W - 2; x++) {
            int v = Max(x, y); if (v > maxBright) continue;
            int a = axis == 0 ? Max(x - 2, y) : Max(x, y - 2), b = axis == 0 ? Max(x + 2, y) : Max(x, y + 2);
            if (v - a >= step && v - b >= step) n[axis == 0 ? x : y]++;
        }
        var s = new System.Text.StringBuilder();
        for (int i = 1; i < len - 1; i++) if (n[i] >= min && n[i] >= n[i - 1] && n[i] >= n[i + 1]) s.Append(i).Append('(').Append(n[i]).Append(") ");
        return s.ToString();
    }
    public string RidgesIn(int axis, int ax, int ay, int aw, int ah, int step, int maxBright, int min) {
        int len = axis == 0 ? W : H; var n = new int[len];
        for (int y = Math.Max(2, ay); y < Math.Min(H - 2, ay + ah); y++) for (int x = Math.Max(2, ax); x < Math.Min(W - 2, ax + aw); x++) {
            int v = Max(x, y); if (v > maxBright) continue;
            int a = axis == 0 ? Max(x - 2, y) : Max(x, y - 2), b = axis == 0 ? Max(x + 2, y) : Max(x, y + 2);
            if (v - a >= step && v - b >= step) n[axis == 0 ? x : y]++;
        }
        var s = new System.Text.StringBuilder();
        for (int i = 1; i < len - 1; i++) if (n[i] >= min && n[i] >= n[i - 1] && n[i] >= n[i + 1]) s.Append(i).Append(' ');
        return s.ToString();
    }
    public int A(int x, int y) { return Px[(y * W + x) * 4 + 3]; }
    // How many pixels have alpha 0, below 128, below 255.
    public string AlphaStats() {
        int z = 0, half = 0, part = 0;
        for (int i = 3; i < Px.Length; i += 4) { if (Px[i] == 0) z++; else if (Px[i] < 128) half++; else if (Px[i] < 255) part++; }
        return "alpha0=" + z + " <128=" + half + " <255=" + part + " of " + (W * H);
    }
    public string AlphaHist(int x0, int y0, int w, int h) {
        var n = new int[16];
        for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++) n[A(x, y) / 16]++;
        return string.Join(" ", n);
    }
    // Dark opaque glyphs (the frame numbers under a VFX cell) in an area: "top;x1-x2,x1-x2,..."
    public string Digits(int ax, int ay, int aw, int ah) {
        var col = new int[aw]; int top = int.MaxValue;
        for (int y = ay; y < ay + ah; y++) for (int x = ax; x < ax + aw; x++) {
            if (A(x, y) >= 200 && Max(x, y) <= 110) { col[x - ax]++; if (y < top) top = y; }
        }
        var s = new System.Text.StringBuilder(); int start = -1;
        for (int i = 0; i <= aw; i++) {
            bool on = i < aw && col[i] > 0;
            if (on && start < 0) start = i;
            if (!on && start >= 0) { if (s.Length > 0) s.Append(','); s.Append(ax + start).Append('-').Append(ax + i - 1); start = -1; }
        }
        return (top == int.MaxValue ? -1 : top) + ";" + s.ToString();
    }
    // Runs of columns holding at least one pixel with alpha >= alphaMin, gaps shorter than `gap`
    // bridged: "x1-x2,x1-x2,..."
    public string Occupancy(int ax, int ay, int aw, int ah, int alphaMin, int gap) {
        var on = new bool[aw];
        for (int x = 0; x < aw; x++) for (int y = ay; y < ay + ah; y++) if (A(ax + x, y) >= alphaMin) { on[x] = true; break; }
        var runs = new List<int[]>(); int start = -1;
        for (int i = 0; i <= aw; i++) {
            bool o = i < aw && on[i];
            if (o && start < 0) start = i;
            if (!o && start >= 0) { runs.Add(new[] { start, i - 1 }); start = -1; }
        }
        var merged = new List<int[]>();
        foreach (var r in runs) { if (merged.Count > 0 && r[0] - merged[merged.Count - 1][1] <= gap) merged[merged.Count - 1][1] = r[1]; else merged.Add(r); }
        var s = new System.Text.StringBuilder();
        foreach (var r in merged) { if (s.Length > 0) s.Append(','); s.Append(ax + r[0]).Append('-').Append(ax + r[1]); }
        return s.ToString();
    }
}
