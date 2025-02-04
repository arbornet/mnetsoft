window.onload   = setfont;
window.onresize = setfont;

function setfont() {
  winW = window.innerWidth;
  winH = window.innerHeight;
  if(winW < 1024)  size  = 1
  if(winW > 1024)  size  = winW * 0.0009
    document.body.style.fontSize = size + 'em'
//  alert("Window width = "+winW+"<br>" +"Window height = "+winH+" size = "+size)
}

