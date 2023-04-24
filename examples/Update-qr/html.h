// literal string
const char* indexHtml = R"literal(
  <!DOCTYPE html>
  <link rel="icon" href='/favicon.ico' sizes="any">
  <link rel="icon" href="/favicon.svg" type="image/svg+xml">
  <body style='width:480px'>
    <h2>ESP Firmware Update</h2>
    <form method='POST' enctype='multipart/form-data' id='upload-form'>
      <input type='file' id= 'file' name='update'>
      <input type='submit' value='Update'>
    </form>
    <br>
    <div id='prg' style='width:0;color:white;text-align:center'>0%</div>
    </body>
    <script>
    var prg = document.getElementById('prg');
    var form = document.getElementById('upload-form');
    form.addEventListener('submit', el=>{
      prg.style.backgroundColor = 'blue'; 
      el.preventDefault();
      var data = new FormData(form);
      var req = new XMLHttpRequest();
      var fsize = document.getElementById('file').files[0].size;
      req.open('POST', '/update?size=' + fsize);  
      req.upload.addEventListener('progress', p=>{
        let w = Math.round(p.loaded/p.total*100) + '%';
          if(p.lengthComputable){
             prg.innerHTML = w;
             prg.style.width = w;
          }
          if(w == '100%') prg.style.backgroundColor = 'black'; 
      });
      req.send(data);
     });
    </script>
)literal";
