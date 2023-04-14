const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">  
  <style>
    body {
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      font-family: 'Dotum', 'Noto Sans KR', Arial, sans-serif;
    }
    .container {
      display: flex;
      flex-direction: column;
      gap: 1rem;
    }
    input[type=text], button {
      padding: 0.5rem;
      font-size: 1rem;
      width: 100%;
      box-sizing: border-box;
    }
    .modal {
      display: none;
      position: fixed;
      z-index: 1;
      width: 100%; 
      height: 100%; 
      background-color: rgba(0, 0, 0, 0.4); 
    }
    .modal-content {
      background-color: #fefefe;
      margin: auto; 
      padding: 20px;
      border: 1px solid #888;
      width: 40%; 
      text-align: center; 
      position: absolute; 
      top: 50%; 
      left: 50%; 
      transform: translate(-50%, -50%);
    }
  </style>
  <script>
    const OPEN  = true;
    const CLOSE = false;
    const SAVE_1 = 0;
    const SAVE_2 = 1;

    const FST_TEXT = 0;
    const SND_TEXT = 1;
    const THD_TEXT = 2;

    var textField_arr = [["ssid",       "password",   ""      ],
                         ["notion_db" , "notion_api", "openai"]];

    var str_label     = [["WIFI Name (SSID)"            , "Password"      , ""              ],
                         ["Link of notion database page", "Notion API key", "OpenAI API key"]];

    function createNewElements(forId, LabelText) {
      var newLabel = document.createElement("label");
      newLabel.setAttribute("for", forId);
      newLabel.textContent = LabelText;
    
      var newInput = document.createElement("input");
      newInput.setAttribute("type", "text");
      newInput.setAttribute("id", forId);
    
      var container = document.querySelector(".container");
      var saveButton = document.querySelector("button"); 
    
      container.insertBefore(newLabel, saveButton);
      container.insertBefore(newInput, saveButton);
    }

    function changeAllForAttribute(prefor, idWord) {
      var labels = document.querySelectorAll("label[for='"+prefor+"']");
    
      labels.forEach(function(label) {
        label.setAttribute("for", idWord);
      });
    }

    function changeLabelText(prefor, newText) {
      var labelElement = document.querySelector("label[for='"+prefor+"']");
    
      labelElement.textContent = newText;
    }

    function changeId(preId,index_2nd,idToChange) {
      var inputElement = document.getElementById(textField_arr[preId][index_2nd]);

      textField_arr[preId][index_2nd] = idToChange;
    
      inputElement.id = idToChange;

      inputElement.value = "";
    }

    function progressModal(openClose){
      var customModal = document.getElementById("customModal");

      if(openClose){
        customModal.style.display = "block";
      }else{
        customModal.style.display = "none";
      }

      return customModal;
    }

    function changeBtnOnClick(onclick) {
      var btn = document.getElementById("saveBtn");

      btn.setAttribute("onclick", onclick);
    }
    
    async function sendData(save) {
      var text_arr = [document.getElementById(textField_arr[save][0]).value,
                      document.getElementById(textField_arr[save][1]).value,
                      save == SAVE_2 ? document.getElementById(textField_arr[save][2]).value : ""];

      for(var i = 0;i<3;i++){
        if(text_arr[i] === ""){
          if(i == 2 && save == SAVE_1) 
            break;
          alert("Empty text");
          return;
        }
      }

      var sendData = "";
      var maxNum = save == SAVE_1 ? 2 : 3;
      for(var i = 0; i <maxNum;i++) {
        if(i != 0) sendData = sendData + "&";
        sendData = sendData + textField_arr[save][i] + "=" + encodeURIComponent(text_arr[i]);
      }
      progressModal(OPEN);

      try{
        const url = save === SAVE_1 ? "/save" : "/save2";
        const response = await fetch(url, {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded",
          },
          body: sendData,
        });

        if (response.ok) {
          progressModal(CLOSE);
    
          if (save === SAVE_1) {
            changeLabelText(textField_arr[save][0],str_label[save+1][0]);
            changeAllForAttribute(textField_arr[save][0],textField_arr[save+1][0]);
            changeId(save,FST_TEXT,textField_arr[save+1][0]);

            document.getElementById(textField_arr[save][1]).type = "text";
            changeLabelText(textField_arr[save][1],str_label[save+1][1]);
            changeAllForAttribute(textField_arr[save][1],textField_arr[save+1][1]);
            changeId(save,SND_TEXT,textField_arr[save+1][1]);            
              
            createNewElements(textField_arr[save+1][2],str_label[save+1][2]);    
          } else if (save === SAVE_2) {
            document.querySelector("#customModal .modal-content p").textContent = "The process is complete.";
            progressModal(OPEN);
          }
        } else {
          throw new Error(`Request failed with status ${response.status}`);
        }
      }catch (error) {
        console.error("Error:", error);
        progressModal(CLOSE);
        alert("Error occurred while sending data");
      }      
      
      if(save == SAVE_1){
        changeBtnOnClick("sendData(SAVE_2)")
      }

      console.log("sendData : " + sendData);
    }
  </script>
</head>
<body>
  <div class="container">
    <label for="ssid"></label>
    <input type="text" id="ssid">
    <label for="password"></label>
    <input type="password" id="password">
    <button id="saveBtn" onclick="sendData(SAVE_1)">Save</button>
  </div>
  <div id="customModal" class="modal">
    <div class="modal-content">
      <p>It's in progress...</p>
    </div>
  </div>
  <script>
    document.querySelector("label[for='ssid']").textContent     = str_label[SAVE_1][0];
    document.querySelector("label[for='password']").textContent = str_label[SAVE_1][1];
  </script>
</body>
</html>
)rawliteral";
