#define TIME_OUT 5000

#define MT1_L 8
#define MT1_R 9

#define MT2_L 10
#define MT2_R 11

struct Sr04 {
    uint8_t trig_pin;
    uint8_t echo_pin;   
};


struct Sr04 srBefore;
struct Sr04 srLeft;
struct Sr04 srRight;   

float GetDistance(struct Sr04 sr04)
{
  long duration, distanceCm;
   
  digitalWrite(sr04.trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(sr04.trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sr04.trig_pin, LOW);
  
  duration = pulseIn(sr04.echo_pin, HIGH, TIME_OUT);

  // convert to distance
  distanceCm = duration / 29.1 / 2;
  
  return distanceCm;
}

void sr_init(){
    srBefore.trig_pin = 2;
    srBefore.echo_pin = 3;
    
    srLeft.trig_pin = 4;
    srLeft.echo_pin = 5;
    
    srRight.trig_pin = 6;
    srRight.echo_pin = 7;
    
    pinMode(srBefore.trig_pin, OUTPUT);
    pinMode(srBefore.echo_pin, INPUT);

    pinMode(srLeft.trig_pin, OUTPUT);
    pinMode(srLeft.echo_pin, INPUT);

    pinMode(srRight.trig_pin, OUTPUT);
    pinMode(srRight.echo_pin, INPUT);
}

void motor_init(){
  pinMode(MT1_L, OUTPUT);
  pinMode(MT1_R, OUTPUT);
  pinMode(MT2_L, OUTPUT);
  pinMode(MT2_R, OUTPUT);
}

void goStraight(){
  digitalWrite(MT1_L, 0);
  digitalWrite(MT1_R, 1);


  digitalWrite(MT2_L, 0);
  digitalWrite(MT2_R, 1);
}

void turnLeft(){


  digitalWrite(MT1_L, 1);
  digitalWrite(MT1_R, 0);

  digitalWrite(MT2_L, 0);
  digitalWrite(MT2_R, 1);
  delay(200);
  stop();
}

void turnRight(){
  digitalWrite(MT1_L, 0);
  digitalWrite(MT1_R, 1);

  digitalWrite(MT2_L, 1);
  digitalWrite(MT2_R, 0);
  delay(200);
  stop();
}

void stop(){
  digitalWrite(MT1_R, 0);
  digitalWrite(MT1_L, 0);

  digitalWrite(MT2_R, 0);
  digitalWrite(MT2_L, 0);
}


void setup() {  
  Serial.begin(9600);
  sr_init();
  motor_init();
  delay(5000);
}

int distanceFront;
int distanceLeft;
int distanceRight;

#define GRID_SIZE 4  // Kích thước ma trận, có thể điều chỉnh theo kích thước khu vực quét
int environment[GRID_SIZE][GRID_SIZE]; // Ma trận môi trường (0: có vật cản, 1: an toàn, 2: đã thăm)

int currentX = 0, currentY = 0;  // Vị trí hiện tại của robot trong ma trận
int direction = 0;  // Hướng di chuyển (0: lên, 1: phải, 2: xuống, 3: trái)

bool isExplored() {
  // Kiểm tra xem tất cả các khu vực an toàn đã được thăm hay chưa
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      if (environment[i][j] == 1) {  // Nếu có khu vực trống chưa được thăm
        return false;  // Chưa khám phá hết
      }
    }
  }
  return true;  // Đã khám phá hết
}

void floodFill(int x, int y) {
  // Nếu vị trí ngoài phạm vi hoặc đã được đánh dấu là đã thăm (hoặc có vật cản), trả về
  if (x < 0 || y < 0 || x >= GRID_SIZE || y >= GRID_SIZE || environment[x][y] == 0) {
    return;
  }
  
  // Đánh dấu khu vực là đã thăm
  environment[x][y] = 2; // Đánh dấu đã thăm (có thể thay bằng giá trị khác để phân biệt)

  // Đánh dấu các vị trí lân cận (Flood Fill 4 hướng)
  floodFill(x + 1, y);  // Di chuyển xuống
  floodFill(x - 1, y);  // Di chuyển lên
  floodFill(x, y + 1);  // Di chuyển phải
  floodFill(x, y - 1);  // Di chuyển trái
}

void SrHandle() {
  distanceFront = GetDistance(srBefore);  // Đọc cảm biến phía trước

  // Cập nhật ma trận môi trường dựa trên cảm biến (1 là an toàn, 0 là có vật cản)
  if (distanceFront > 10) {
    environment[currentX][currentY] = 1;  // Nếu không có vật cản phía trước, đánh dấu an toàn
  } else {
    environment[currentX][currentY] = 0;  // Nếu có vật cản, đánh dấu có vật cản
  }


  // Nếu có vật cản phía trước
  if (distanceFront < 20 && distanceFront != 0) {
    stop();  // Dừng lại để xử lý
    distanceLeft  = GetDistance(srLeft);    // Đọc cảm biến trái
    distanceRight = GetDistance(srRight);   // Đọc cảm biến phải


    // Kiểm tra xem có vật cản bên trái không
    if (distanceLeft > 15 || distanceLeft == 0) {
      // Nếu có không gian an toàn ở bên trái, quay sang phải để tránh vật cản phía trước
      delay(500);
      turnRight();  // Quay phải
      delay(500);
      // Cập nhật ma trận Flood Fill sau khi di chuyển
      floodFill(currentX + 1, currentY);  // Cập nhật sau khi di chuyển sang phải
      goStraight();
    } 
    // Kiểm tra vật cản bên phải
    else if (distanceRight > 15 || distanceRight == 0) {
      // Nếu có không gian an toàn ở bên phải, quay sang trái để tránh vật cản phía trước
      delay(500);
      turnLeft();   // Quay trái
      delay(500);
      // Cập nhật ma trận Flood Fill sau khi di chuyển
      floodFill(currentX - 1, currentY);  // Cập nhật sau khi di chuyển sang trái
      goStraight();
    } 
    // Nếu cả hai bên trái và phải đều có vật cản, quay lại hoặc tránh theo một hướng khác
    else {

    }
  } 
  // Nếu không có vật cản phía trước, tiếp tục đi thẳng
  else {
    goStraight();
  }
}


void loop() {
  SrHandle();
  //goStraight();

} 
