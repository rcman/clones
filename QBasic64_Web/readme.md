<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>QuickBASIC 64</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: monospace; background: #0000AA; color: white; height: 100vh; display: flex; flex-direction: column; }
.menu { background: #C0C0C0; color: black; padding: 5px 10px; border-bottom: 2px solid #808080; }
.toolbar { background: #D4D0C8; padding: 5px 10px; border-bottom: 2px solid #808080; }
.toolbar button { padding: 5px 15px; margin-right: 5px; cursor: pointer; }
.main { display: flex; flex: 1; overflow: hidden; }
.left { flex: 1; display: flex; flex-direction: column; border-right: 2px solid #404040; }
.right { flex: 1; display: flex; flex-direction: column; background: black; }
.title { background: #0000AA; padding: 5px 10px; font-size: 11px; border-bottom: 1px solid #404040; }
#editor { flex: 1; background: #0000AA; color: yellow; padding: 15px; font-size: 14px; border: none; resize: none; outline: none; }
.canvas-box { flex: 1; display: flex; align-items: center; justify-content: center; padding: 20px; }
#canvas { border: 2px solid #404040; image-rendering: pixelated; }
.status { background: #D4D0C8; color: black; padding: 5px 10px; font-size: 11px; display: flex; justify-content: space-between; }
</style>
</head>
<body>
<div class="menu"><b>QuickBASIC 64</b> File Edit View Run Help</div>
<div class="toolbar">
<button id="run" style="background: green; color: white;">Run F5</button>
<button id="stop" style="background: red; color: white;">Stop</button>
<button id="clear" style="background: blue; color: white;">Clear</button>
</div>
<div class="main">
<div class="left">
<div class="title">UNTITLED.BAS</div>
<textarea id="editor" spellcheck="false">REM QuickBASIC 64 Test
SCREEN 13
CLS
PRINT "Drawing pixels..."
FOR i = 0 TO 50
  PSET (50 + i, 50), 14
NEXT i
PRINT "Drawing circle..."
CIRCLE (160, 100), 40, 15
PRINT "Drawing lines..."
LINE (10, 150)-(100, 150), 10
LINE (100, 150)-(100, 190), 12
PRINT "Drawing spiral..."
FOR a = 0 TO 100
  x = 240 + 50 * COS(a / 10)
  y = 140 + 50 * SIN(a / 10)
  PSET (x, y), a MOD 16
NEXT a
PRINT "Done!"</textarea>
</div>
<div class="right">
<div class="title">OUTPUT</div>
<div class="canvas-box"><canvas id="canvas" width="640" height="400"></canvas></div>
</div>
</div>
<div class="status"><span id="msg">Ready</span><span id="pos">Ln 1 Col 1</span></div>
<script>
var c = document.getElementById('canvas');
var ctx = c.getContext('2d');
var ed = document.getElementById('editor');
var msg = document.getElementById('msg');
var running = false;
var stop = false;
var mode = 0;
var tx = 0, ty = 0;
var cols = ['#000','#00A','#0A0','#0AA','#A00','#A0A','#A50','#AAA','#555','#55F','#5F5','#5FF','#F55','#F5F','#FF5','#FFF'];

function clr() {
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, c.width, c.height);
}

function wait(ms) {
  return new Promise(function(r) { setTimeout(r, ms); });
}

function run() {
  if (running) return;
  running = true;
  stop = false;
  document.getElementById('run').disabled = true;
  msg.textContent = 'Running...';
  clr();
  
  var code = ed.value;
  var lines = code.split('\n');
  var vars = {};
  var i = 0;
  
  function step() {
    if (i >= lines.length || stop) {
      msg.textContent = stop ? 'Stopped' : 'Done';
      running = false;
      document.getElementById('run').disabled = false;
      return;
    }
    
    var line = lines[i].trim();
    
    if (!line || line.toUpperCase().indexOf('REM') === 0 || line.indexOf("'") === 0) {
      i++;
      setTimeout(step, 1);
      return;
    }
    
    if (line.toUpperCase().indexOf('FOR') === 0) {
      var m = line.match(/FOR\s+(\w+)\s*=\s*(.+?)\s+TO\s+(.+?)(?:\s+STEP\s+(.+))?$/i);
      if (m) {
        var v = m[1];
        var s = calc(m[2], vars);
        var e = calc(m[3], vars);
        var st = m[4] ? calc(m[4], vars) : 1;
        var ls = i + 1;
        var le = i;
        var d = 1;
        for (var j = i + 1; j < lines.length; j++) {
          var t = lines[j].trim().toUpperCase();
          if (t.indexOf('FOR') === 0 && t.indexOf('FORMAT') !== 0) d++;
          if (t.indexOf('NEXT') === 0) {
            d--;
            if (d === 0) { le = j; break; }
          }
        }
        var lv = s;
        function loop() {
          if (stop || (st > 0 ? lv > e : lv < e)) {
            i = le + 1;
            setTimeout(step, 1);
            return;
          }
          vars[v] = lv;
          var bi = ls;
          function body() {
            if (bi >= le || stop) {
              lv += st;
              setTimeout(loop, 1);
              return;
            }
            exec(lines[bi].trim(), vars);
            bi++;
            setTimeout(body, 1);
          }
          body();
        }
        loop();
        return;
      }
    }
    
    exec(line, vars);
    i++;
    setTimeout(step, 10);
  }
  
  step();
}

function parseArgs(str) {
  // Remove outer parentheses and split by comma, respecting nested parens
  str = str.trim();
  var args = [];
  var depth = 0;
  var current = '';
  for (var i = 0; i < str.length; i++) {
    var ch = str[i];
    if (ch === '(' || ch === ')') {
      depth += (ch === '(' ? 1 : -1);
      current += ch;
    } else if (ch === ',' && depth === 0) {
      args.push(current.trim());
      current = '';
    } else {
      current += ch;
    }
  }
  if (current.trim()) args.push(current.trim());
  return args;
}

function exec(line, vars) {
  // Skip NEXT statements (handled by FOR)
  if (line.toUpperCase().indexOf('NEXT') === 0) return;
  
  // Variable assignment: x = expr
  if (line.match(/^[a-zA-Z_]\w*\s*=/) && !line.toUpperCase().match(/^(SCREEN|CLS|PSET|LINE|CIRCLE|PRINT|LOCATE)/)) {
    var p = line.indexOf('=');
    var n = line.substring(0, p).trim();
    var x = line.substring(p + 1).trim();
    vars[n] = calc(x, vars);
    return;
  }
  
  var u = line.toUpperCase();
  
  if (u.indexOf('SCREEN') === 0) {
    var m = parseInt(line.substring(6).trim());
    mode = m;
    c.width = m === 13 ? 320 : 640;
    c.height = m === 13 ? 200 : 400;
    clr();
  } else if (u.indexOf('CLS') === 0) {
    clr();
    tx = 0; ty = 0;
  } else if (u.indexOf('PSET') === 0) {
    // PSET (x, y), color  OR  PSET x, y, color
    var rest = line.substring(4).trim();
    var x, y, col;
    
    if (rest[0] === '(') {
      // Format: PSET (x, y), color
      var closeParen = rest.indexOf(')');
      var coords = rest.substring(1, closeParen);
      var coordParts = coords.split(',');
      x = calc(coordParts[0], vars);
      y = calc(coordParts[1], vars);
      var afterParen = rest.substring(closeParen + 1).trim();
      if (afterParen[0] === ',') {
        col = calc(afterParen.substring(1), vars);
      } else {
        col = 15;
      }
    } else {
      // Format: PSET x, y, color
      var args = parseArgs(rest);
      x = calc(args[0], vars);
      y = calc(args[1], vars);
      col = args[2] ? calc(args[2], vars) : 15;
    }
    
    var sc = mode === 13 ? 2 : 1;
    ctx.fillStyle = cols[Math.floor(col) % 16];
    ctx.fillRect(Math.floor(x) * sc, Math.floor(y) * sc, sc, sc);
  } else if (u.indexOf('LINE') === 0) {
    // LINE (x1, y1)-(x2, y2), color  OR  LINE x1, y1, x2, y2, color
    var rest = line.substring(4).trim();
    var x1, y1, x2, y2, col;
    
    if (rest[0] === '(') {
      // Format: LINE (x1, y1)-(x2, y2), color
      var dashPos = rest.indexOf('-');
      var firstCoords = rest.substring(1, rest.indexOf(')'));
      var firstParts = firstCoords.split(',');
      x1 = calc(firstParts[0], vars);
      y1 = calc(firstParts[1], vars);
      
      var afterDash = rest.substring(dashPos + 1).trim();
      var secondStart = afterDash.indexOf('(') + 1;
      var secondEnd = afterDash.indexOf(')');
      var secondCoords = afterDash.substring(secondStart, secondEnd);
      var secondParts = secondCoords.split(',');
      x2 = calc(secondParts[0], vars);
      y2 = calc(secondParts[1], vars);
      
      var afterSecond = afterDash.substring(secondEnd + 1).trim();
      if (afterSecond[0] === ',') {
        col = calc(afterSecond.substring(1).split(',')[0], vars);
      } else {
        col = 15;
      }
    } else {
      // Format: LINE x1, y1, x2, y2, color
      var args = parseArgs(rest);
      x1 = calc(args[0], vars);
      y1 = calc(args[1], vars);
      x2 = calc(args[2], vars);
      y2 = calc(args[3], vars);
      col = args[4] ? calc(args[4], vars) : 15;
    }
    
    var sc = mode === 13 ? 2 : 1;
    ctx.strokeStyle = cols[Math.floor(col) % 16];
    ctx.lineWidth = sc;
    ctx.beginPath();
    ctx.moveTo(Math.floor(x1) * sc + 0.5, Math.floor(y1) * sc + 0.5);
    ctx.lineTo(Math.floor(x2) * sc + 0.5, Math.floor(y2) * sc + 0.5);
    ctx.stroke();
  } else if (u.indexOf('CIRCLE') === 0) {
    // CIRCLE (x, y), radius, color  OR  CIRCLE x, y, radius, color
    var rest = line.substring(6).trim();
    var x, y, r, col;
    
    if (rest[0] === '(') {
      // Format: CIRCLE (x, y), radius, color
      var closeParen = rest.indexOf(')');
      var coords = rest.substring(1, closeParen);
      var coordParts = coords.split(',');
      x = calc(coordParts[0], vars);
      y = calc(coordParts[1], vars);
      
      var afterParen = rest.substring(closeParen + 1).trim();
      if (afterParen[0] === ',') {
        var remaining = parseArgs(afterParen.substring(1));
        r = calc(remaining[0], vars);
        col = remaining[1] ? calc(remaining[1], vars) : 15;
      }
    } else {
      // Format: CIRCLE x, y, radius, color
      var args = parseArgs(rest);
      x = calc(args[0], vars);
      y = calc(args[1], vars);
      r = calc(args[2], vars);
      col = args[3] ? calc(args[3], vars) : 15;
    }
    
    var sc = mode === 13 ? 2 : 1;
    ctx.strokeStyle = cols[Math.floor(col) % 16];
    ctx.lineWidth = sc;
    ctx.beginPath();
    ctx.arc(Math.floor(x) * sc, Math.floor(y) * sc, Math.floor(r) * sc, 0, Math.PI * 2);
    ctx.stroke();
  } else if (u.indexOf('PRINT') === 0) {
    var txt = line.substring(5).trim();
    // Remove quotes
    txt = txt.replace(/^["']|["']$/g, '');
    ctx.fillStyle = '#FFF';
    ctx.font = (mode === 13 ? '16' : '16') + 'px monospace';
    ctx.fillText(txt, tx, ty + 16);
    ty += 16;
  } else if (u.indexOf('LOCATE') === 0) {
    var args = parseArgs(line.substring(6));
    ty = (parseInt(args[0]) - 1) * 16;
    tx = (parseInt(args[1]) - 1) * 8;
  }
}

function calc(expr, vars) {
  if (expr === undefined || expr === null) return 0;
  expr = String(expr).trim();
  
  // Replace variable names with their values
  expr = expr.replace(/\b([a-zA-Z_]\w*)\b/g, function(m) {
    var upper = m.toUpperCase();
    if (['COS','SIN','TAN','ATN','SQR','ABS','INT','RND','MOD','TO','STEP','AND','OR','NOT'].indexOf(upper) !== -1) {
      return m;
    }
    return vars[m] !== undefined ? vars[m] : m;
  });
  
  // Replace BASIC functions with JS equivalents
  expr = expr.replace(/COS\s*\(/gi, 'Math.cos(');
  expr = expr.replace(/SIN\s*\(/gi, 'Math.sin(');
  expr = expr.replace(/TAN\s*\(/gi, 'Math.tan(');
  expr = expr.replace(/ATN\s*\(/gi, 'Math.atan(');
  expr = expr.replace(/SQR\s*\(/gi, 'Math.sqrt(');
  expr = expr.replace(/ABS\s*\(/gi, 'Math.abs(');
  expr = expr.replace(/INT\s*\(/gi, 'Math.floor(');
  expr = expr.replace(/\bMOD\b/gi, '%');
  
  try {
    return eval(expr);
  } catch (e) {
    console.error('Calc error:', expr, e);
    return 0;
  }
}

document.getElementById('run').onclick = run;
document.getElementById('stop').onclick = function() { stop = true; };
document.getElementById('clear').onclick = function() { clr(); msg.textContent = 'Cleared'; };
document.onkeydown = function(e) { if (e.key === 'F5') { e.preventDefault(); run(); } };
ed.onclick = ed.onkeyup = function() {
  var l = ed.value.substring(0, ed.selectionStart).split('\n');
  document.getElementById('pos').textContent = 'Ln ' + l.length + ' Col ' + (l[l.length-1].length + 1);
};
clr();
</script>
</body>
</html>
