let serial;
let brightnessSlider;
let button1, button2, button3;
let trafficState = true;  // 신호등 동작 여부
let b1State = false;
let b2State = false;
let b3State = true;

function setup() {
    createCanvas(500, 400);
    background(240);

    // p5.webserial 객체 생성
    serial = new p5.WebSerial();

    // 시리얼 이벤트 설정
    serial.on("data", serialEvent);
    serial.on("error", serialError);

    // 웹 시리얼 연결 버튼
    let connectButton = select("#connectButton");
    connectButton.mousePressed(connectToSerial);

    // 밝기 조절 슬라이더
    brightnessSlider = createSlider(0, 255, 255);
    brightnessSlider.position(30, 50);
    brightnessSlider.input(sendBrightness);

    // 버튼 1: 빨간불 켜기 / 신호등 다시 시작
    button1 = createButton("버튼 1 (빨간불)");
    button1.position(30, 100);
    button1.mousePressed(button1Click);

    // 버튼 2: 모든 LED 깜빡이기
    button2 = createButton("버튼 2 (LED 깜빡임)");
    button2.position(30, 140);
    button2.mousePressed(button2Click);

    // 버튼 3: 신호등 기능 재시작
    button3 = createButton("버튼 3 (신호등 재시작)");
    button3.position(30, 180);
    button3.mousePressed(button3Click);
}

function draw() {
    background(240);
    textSize(16);
    fill(0);
    
    text(`밝기: ${brightnessSlider.value()}`, 30, 40);
    text(`현재 신호등: ${trafficState ? "작동 중" : "정지됨"}`, 30, 220);
}

// 🔹 아두이노 연결 요청 (사용자가 직접 포트 선택)
function connectToSerial() {
    if (!serial.opened()) {
        serial.requestPort();  // 사용자에게 포트 선택 창 표시
        serial.open();  // 선택된 포트 열기
        console.log("🔗 시리얼 포트 연결 요청");
    } else {
        console.log("⚠️ 이미 연결되어 있습니다.");
    }
}

// 🔹 시리얼 데이터 수신
function serialEvent() {
    let data = serial.readLine();
    if (data && data.trim().length > 0) {
        console.log("📩 아두이노에서 수신됨:", data);
    }
}

// 🔹 밝기 값 아두이노로 전송
function sendBrightness() {
    let b = brightnessSlider.value();
    serial.write("B" + b + "\n");
    console.log("💡 밝기 변경: " + b);
}

// 🔹 버튼 1 클릭: 빨간불 켜기 / 신호등 재시작
function button1Click() {
    b1State = !b1State;
    if (b1State) {
        console.log("버튼 1: 빨간불 켜기");
        serial.write("r\n");
    } else {
        console.log("버튼 1: 신호등 재시작");
        serial.write("1\n");
    }
}

// 🔹 버튼 2 클릭: 모든 LED 깜빡이기
function button2Click() {
    b2State = !b2State;
    if (b2State) {
        console.log("버튼 2: 모든 LED 깜빡이기");
        serial.write("b\n");
    } else {
        console.log("버튼 2: 신호등 재시작");
        serial.write("1\n");
    }
}

// 🔹 버튼 3 클릭: 신호등 기능 재시작
function button3Click() {
    console.log("버튼 3: 신호등 기능 재시작");
    serial.write("1\n");
}

// 🔹 시리얼 오류 처리
function serialError(err) {
    console.error("⚠️ 시리얼 오류 발생:", err);
}
