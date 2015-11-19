/*************************************************************************************
Giang Do (Đỗ Trường Giang), November 2015
 
Đây là chương trình Pomodoro Timer dùng hai bo mạch lcd-button shield và Ardunio U3

Chương trình gồm có 3 mode chính:
	+ Countdown(đếm lùi): đây là mode chính của chương trình, cho phép thiết lập thời
		gian đếm lùi theo 3 kiểu và phát tín hiệu cảnh báo khi thời gian lùi về 0.
			- Working : Làm việc
			- Long Break : Nghỉ dài
			- Short Break : Nghỉ ngắn
	+ Summary(Tổng hợp): đây là mode dùng để hiển thị kết quả số lẩn thực hiện thành công của
		từng kiểu đếm lùi. từ khi đồng hồ pomodoro này được khởi động chạy.

	+ Modify(Điều chỉnh): đây là mode dùng để điểu chỉnh khoảng thời gian cho mỗi kiểu đếm lùi.
		Giá trị mặc định là:
		- Working: 25 phút
		- Long Break: 15 phút
		- Short Break: 5 phút 

Cách sử dụng:
	+ Nhấn Select để chuyển mode: Countdown -> Summary -> Modify -> Countdown
	+ Trong mode Countdown:
		- Để bắt đầu đếm lùi Nhấn nút Work, hoặc LBreak, hoặc SBreak để bắt đầu đếm lùi
		   theo kiểu Làm Việc, Nghỉ dài, nghỉ ngắn.
		- Nhấn nút Fail để xác nhận chu kỳ đang thực hiện cũng như vừa thực hiện xong ko thành công.

	+ Trong mode Summary: xem số lần làm việc/nghỉ ngắn/nghỉ dài thành công
		- Nhấn nút Fail để thiết lập lại tất cả giá trị các bộ đếm về 0

	+ Trong mode Modify: 
		- Nhấn nút Work, hoặc LBreak, hoặc SBreakWork chỉnh giá trị cho thời gian Làm việc,
		  Nghỉ dài, Nghỉ ngắn

Thế nào là một chu kỳ thành công ?
	+ Một chu kì chỉ được xác định là thành công chỉ khi đồng hồ đếm lùi đã về 0
	  và người dùng ko nhấn nút Fail sau đó.

Thế nào là một chu kỳ ko thành công ?
	+ Trong lúc đang đếm lùi: Nhấn bất kỳ một nút nào đó
	+ Khi đã kết thúc: Nhấn nút Fail.
			
* Định hướng lập trình (dành cho các bạn mới làm quen với lập trình nhúng)
	+ Cần phải giải quyết bài toán MCU làm nhiều công việc gần như cùng lúc (lập trình song song):
		- MCU phải điều khiển chip LCD driver để hiển thị ký tự trên LCD
		- MCU phải đọc giá trị ADC để biết nút nhấn nào được nhấn
		- MCU phải tăng giá trị timer để làm bộ đếm lùi
		- MCU phải tính toán để quyết định hiển thị cái gì ra màn hình.

		-> Hướng giải quyết:
		 	. Các công việc tất nhiên sẽ được thực hiện 1 cách tuần tự. (vì ko có nhiều core trong MCU)
			. Các công việc phải được chia rất nhỏ (thời gian thực hiện rất ngắn, tính bằng micro giấy)
		 	. Ko thể để một công việc chiếm quá nhiều thời gian xử lý khiến cho tất cả các chức năng
			  khác ko thể thực thi.
			  Ví dụ: Chắc chắn là khi MCU điều khiển LCD, MCU sẽ ko thể nhận biết được nút nhấn.
			  			Vậy nếu ta cách ta lập trình khiến cho MCU tốn nhiều thời gian cho điều khiển LCD,
						MCU sẽ hầu như ko nhận được tín hiệu nhấn nút được do lúc ta nhấn nút
						thì MCU đang bận điều khiển LCD rồi.

	+ Cần phải giải quyết bài toán máy trạng thái:
		- Chỉ cần nhấn một nút Select thì có thể chuyển qua lần lượt 3 mode, tức là chuyển trạng
		  thái chỉ thông qua một nút nhấn chứ ko phải là mỗi nút nhấn một trạng thái.

		- Ngoài ra ở mỗi mode, nút nhấn cũng có chức năng khác nhau.

		-> Hướng giải quyết:
			. Dùng bảng để thể hiện máy trạng thái.
			. Dùng con trỏ hàm để thể hiện chức năng của mỗi nút nhấn.

**************************************************************************************/
 
/* Khai báo sử dụng thư viện điều khiển LCD */
#include <LiquidCrystal.h>
 
// Kỹ thuật sử dụng macro để tránh sử dụng những con số vô nghĩa trong lập trình
// XXX please check this again
#define LCD_D0 8  //GPIO8 của MCU nối với chân LCD D0
#define LCD_D1 9  //GPIO9 của MCU nối với chân LCD D1
#define LCD_D2 4  //GPIO4 của MCU nối với chân LCD D2
#define LCD_D3 5  //GPIO5 của MCU nối với chân LCD D3
#define LCD_D4 6  //GPIO6 của MCU nối với chân LCD D4
#define LCD_D5 7  //GPIO7 của MCU nối với chân LCD D5
#define COLUM_0 0
#define COLUM_1 1
#define NUMBER_OF_COLUM 16
#define LINE_0 0
#define LINE_1 1
#define NUMBER_OF_LINE 2
// Khai báo một object của lớp LiquidCrystal, đồng thời thông qua constructor của lớp
// này xác định các chân GPIO trên MCU dùng để điều khiển LCD 
// Thư viện LiquidCrystal viết bằng C++ ở arduino-1.6.6/libraries/LiquidCrystal/src/LiquidCrystal.h 
LiquidCrystal lcd(LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5);

// Kỹ thuật sử dụng enum để tránh sử dụng những con số vô nghĩa trong lập trình 
typedef enum button
{
	selectB = 0, // Select Button      <=> nút Select ở board lcd shield
	stopB   = 1, // Stop Button        <=> nút Left   ở board lcd shield
	workB   = 2, // Work Button        <=> nút Up     ở board lcd shield
	sBreakB = 3, // Short Break Button <=> nút Down   ở board lcd shield
	lBreakB = 4, // Long Break Button  <=> nút Right  ở board lcd shield
   noneB	  = 5  // Không có nút nào được nhấn
}E_Button; //Enum Button

#define adcPIN 0 //Chần GPIO 0 của MCU được nối với bộ nút nhấn của board LCD shield

// Hàm này dùng để xác định nút đã được nhấn trên board lcd shield
E_Button read_buttons(){
	// MCU đọc giá trị của hiệu điện thế từ chân ADC (Analog Digital Converter)
	// hàm analogRead() được khai báo trong arduino-1.6.6/hardware/arduino/avr/cores/arduino/wiring_analog.c
    int adc = analogRead(adcPIN); 

	// Thủ thuật hay: Bằng cách nối các nút nhấn vào một chân ADC của MCU như trong board
	// LCD shield này ta có thể xác định được trạng thái của nút nhấn thông qua giá trị
	// hiệu điện thế của một chân MCU. -> Tiết kiệm được chân MCU vì cách thông thường là
	// mỗi nút nhấn sẽ được kiểm soát bởi một chân MCU.
		 
 	 // Khi adc là rất lớn ,gần như vô cùng -> ko có nút nào được nhấn
    if (adc > 1000) return noneB; 
 
	 // Nếu dùng board LCD shield version 1.0 thì dùng bảng sau xóa bảng còn lại
    if (adc < 50)  return lBreakB; //Khi nhấn nút lBreak  giá trị chính xác trả về là 0
    if (adc < 195) return workB;   //Khi nhấn nút workB   giá trị chính xác trả về là 144 
    if (adc < 380) return sBreakB; //Khi nhấn nút sBreakB giá trị chính xác trả về là 329 
    if (adc < 555) return stopB;   //Khi nhấn nút stopB   giá trị chính xác trả về là 504 
    if (adc < 790) return selectB; //Khi nhấn nút selectB giá trị chính xác trả về là 741 
 
	 // Nếu dùng board LCD shield version 1.1 thì dùng bảng sau xóa bảng còn lại
    // if (adc_key_in < 50)   return lBreakB;  
    // if (adc_key_in < 250)  return workB; 
    // if (adc_key_in < 450)  return sBreakB; 
    // if (adc_key_in < 650)  return failB; 
    // if (adc_key_in < 850)  return selectB;

    return noneB;
}

unsigned long prevTime = 0;
typedef enum state
{
  workState   = 0,  //Working state
  sBreakState = 1,  //Short Break state
  lBreakState = 2,  //Long Break state 
  stopState   = 3,  //Stop state
  notifyState = 4   // Notify state
}E_State; // Enum State

unsigned long curSecond = 0;
unsigned long curMin = 0;
unsigned long resMin = 0;
  
typedef enum mode
{
	countDownM = 0, // Countdown Mode
	summaryM   = 1, // Summary Mode
   modifyM    = 2, // Modify Mode
   noneM      = 3  // None Mode, this is use as polar
}E_Mode; // Enum mode

typedef void (*buttonHdl) (eButton button,void *data);
typedef void (*modeHdl) (void* data);
typedef struct mode
{
	E_Mode mode; //Mode
	buttonHdl buttonHandler; //function pointer that point to button handler function   	
	modeHdl taskHandler; //Function pointer that point to normal task handler of this mode
}T_Mode;

//Interface between mode and main thread
typedef struct lcdBuffer
{
	char lcdBuf[2][17]; //
	char lcdBufBlink[2][17];  //XXX should change to bit technique here
	char lcdBufPrev[2][17];
}T_LcdBuffer g_LcdBuffer;

typedef enum bool
{
	False = 0,
	True  = 1
}E_Bool;
typedef struct countDown 
{
	unsigned long prevTime;
	E_State state;
	E_Bool countDone;
	unsigned long mileStone;
	unsigned char remain; // Thời gian còn lại tính theo đơn vị phút 
	char prevState[17];
}T_CountDown;

typedef struct count
{
	unsigned char period; // Thời gian đếm lùi tính theo đơn vị phút

	unsigned char success; // Tồng số lần thành công
	unsigned char fail;	  // Tổng số lần thất bại
	char const * const progressStr; //Chuỗi sẽ in ra khi đang trong quá trình đếm
	char const * const doneStr;     //Chuỗi sẽ in ra khi đếm xong
	char const * const str;         //Tên của loại đếm xuống
}T_Count;

typedef struct dataBlock
{
	T_CountDown countDownData;
	T_Count     work;
	T_Count     sBreak;
	T_Count     lBreak;
   E_Mode      curMode;
}T_DataBlock g_DataBlock;

void countDownButtonHdl(E_Button button);
void countDownTaskHdl();
void summaryButtonHdl(E_Button button);
void summaryTaskHdl();
void modifyButtonHdl(E_Button button);
void modifyTaskHdl();

// What is the mechanism to transfer data between mode
T_Mode g_Mode[] = // global variable mode
{
	{countDownM , countDownButtonHdl , countDownTaskHdl},
	{summaryM   , summaryButtonHdl   , summaryTaskHdl  },
	{modifyM    , modififyButtonHdl  , modifyTaskHdl   }
}

void summaryButtonHdl(E_Button button)
{
	T_Count *pT_Count = NULL;
	switch (button)
	{
		case selectB:
		{
			g_DataBlock.curMode = modifyM;
			break;
		}
		case workB:
		{
			pT_Count = &g_DataBlock.work;
			break;
		}
		case sBreakB:
		{
			pT_Count = &g_DataBlock.sBreak;
			break;
		}
		case lBreakB:
		{
			pT_Count = &g_DataBlock.sBreak;
			break;
		}
		case Stop:
		case default:
			{
				break;
			}
	}
	if (pT_Count != NULL)
	{
		pT_Count.success = 0;
		pT_Count.fail = 0;
	}
}

void summaryTaskHdl()
{
	// Print counter
}

void modifyButtonHdl(E_Button button)
{
	T_Count *pT_Count = NULL;
	switch (button)
	{
		case selectB:
		{
			g_DataBlock.curMode = modifyM;
			break;
		}
		case workB:
		{
			pT_Count = &g_DataBlock.work;
			break;
		}
		case sBreakB:
		{
			pT_Count = &g_DataBlock.sBreak;
			break;
		}
		case lBreakB:
		{
			pT_Count = &g_DataBlock.sBreak;
			break;
		}
		case Stop:
		case default:
			{
				break;
			}
	}
	if (pT_Count != NULL)
	{
		pT_Count.period--;
		if (pT_Count.period == 1)
		{
			pT_Count.period = MAX_PERIOD_MINUTE;
		}
	}
}

void modifyTaskHdl()
{
	// print period 
}

void countDownButtonHdl(E_Button button)
{
	if (button == noneB)
	{
		// ko có nút nhấn, ko làm gì cả
	}
	else
	{
		// Xác định chu kỳ hiện tại đã thành công hay chưa
		T_Count *pT_Count = NULL;
		if (g_DataBlock.countDownData.state == workState)
		{
			pT_Count = &g_DataBlock.work;
		}
		else if (g_DataBlock.countDownData.state == sBreakState)
		{
			pT_Count = &g_DataBlock.sBreak;
		}
		else if (g_DataBlock.countDownData.state == lBreakState)
		{
			pT_Count = &g_DataBlock.lBreak;
		}
		if (pT_Count != NULL)
		{
			if (g_DataBlock.countDownData.remain == 0)
			{
				pT_Count->success++; // Chu kỳ thành công vì đã đếm hết về 0
			}
			else
			{
				pT_Count->fail++;
			}
		}

		// Xử lý công việc theo chức năng của nút nhấn
		if (button == selectB)
		{
			g_DataBlock.curMode = summaryM;
			g_DataBlock.countDownData.state = stopState; // Xác định trạng thái khi quay lại mode countdown này
			g_DataBlock.countDownData.remain = 0;
		}
		else 
		{
			g_DataBlock.countDownData.mileStone = millis();
			if (button == stopB)
			{
				g_DataBlock.countDownData.state = stopState;
				g_DataBlock.countDownData.remain = 0;
			}
			else if (button == workB)
			{
				g_DataBlock.countDownData.state = workState;
				g_DataBlock.countDownData.remain = g_DataBlock.work.period;
			}
			else if (button == sBreak)
			{
				g_DataBlock.countDownData.state = sBreakState;
				g_DataBlock.countDownData.remain = g_DataBlock.sBreak.period;
			}
			else if (button == lBreak)
			{
				g_DataBlock.countDownData.state = lBreakState;
				g_DataBlock.countDownData.remain = g_DataBlock.lBreak.period;
			}
		}
	}
}

void countDownTaskHdl()
{
	// Tìm pointer trỏ đến state đếm hiện tại
	T_Count *pT_Count = NULL;
	if (g_DataBlock.countDownData.state == workState)
	{
		pT_Count = &g_DataBlock.work;
	}
	else if (g_DataBlock.countDownData.state == sBreakState)
	{
		pT_Count = &g_DataBlock.sBreak;
	}
	else if (g_DataBlock.countDownData.state == lBreakState)
	{
		pT_Count = &g_DataBlock.lBreak;
	}

	if (pT_Count != NULL) // Kiểm tra xem có đang ở state đếm nào ko ?
	{
		// Lưu ý là hàm millis() trả về giá trị tăng dần của timer, vì kiểu trả về là unsigned long -> giá trị
		// trả về lớn nhất của hàm là 2^64 micro giấy -> dùng hàm này ta có thể đếm từ bây giờ đến khoảng năm
		// trăm ngàn thiên niên kỉ nữa!

		// Số giây đã trải qua từ lúc bấm nút cho đếm
		unsigned long secPassed = (millis() - g_DataBlock.countDownData.mileStone) / SECOND_TO_MICROSECONDS;
		// Số phút đã trải qua từ lúc bấm nút cho đếm
		unsigned long minPassed = secPassed / MINUTE_TO_SECONDS;
		if (minPassed <= pT_Count->period)
		{
			g_DataBlock.countDownData.remain = pT_Count->period - minPassed;
			lcd_buffer_insert(LINE_1, 0, pT_Count->progressStr, False);
			//lcd_buffer_insert(LINE_1, 17, num2Str(g_DataBlock.countDownData.remain), False);
		}
		else
		{
			lcd_buffer_insert(LINE_0, 0, pT_Count->doneStr, True);
			lcd_buffer_insert(LINE_1, 0, "Please " , True);
			//lcd_buffer_insert(LINE_1, 7,  recommended_next_state(); True);
		}
	}
	else // Ko có pointer => đang ở Stop State
	{
		lcd_buffer_insert(LINE_0, 0, "Let pomodoro", False);
		lcd_buffer_insert(LINE_1, 0, "Let do it!" , False);
	}
}

void setup()
{
	// Khởi động LCD dùng hàm void LiquidCrystal::begin(uint8_t cols, uint8_t lines, uint8_t dotsize)
	// và dùng kỹ thuật default argument trong C++ để không cần cung cấp thông tin về dotsize
	// mà lấy giá trị mặc định LCD_5x8DOTS.
	// LCD_5x8DOTS5x8 có ý nghĩa là lcd dùng ô chữ chật 5x8 = 40 điểm để hiển thị 1 ký tự
	lcd.begin(NUMBER_OF_COLUM, NUMBER_OF_LINE);


	lcd.setCursor(COLUM_0,LINE_0);  // di chuyển con trỏ của LCD đến dòng đầu tiên, cột đầu tiên

	// Dùng hàm size_t Print::print(const char str[]) ở arduino-1.6.6/hardware/arduino/avr/cores/arduino/Print.cpp
	// để in dòng chữ "Wellcome"
	// Kỹ thuật inheritance trong C++ được sử dụng, đối tượng của lớp LiquidCrystal có thể gọi hàm của lớp Print
	// vì lớp LiquidCrystal thừa kế theo kiểu public lớp Print
	lcd.print("Welcome");    

	// In số 0 rồi 1 rồi 2 ... 
	for (int i = 0; i < 3, i++)
	{
		lcd.setCursor(COLUM_0,LINE_1);
		lcd.print(i);  //XXX need to comment here
		delay(1000); 	//dừng 1000ms, hàm này ở arduino-1.6.6/hardware/arduino/avr/cores/arduino/wiring.c
	}

	// In "Pomodoro Now!"
	char string[12];
	string[0]  = 'P';
	string[1]  = 'o';
	string[2]  = 'm';
	string[3]  = 'o';
	string[4]  = 'd';
	string[5]  = 'o';
	string[6]  = 'r';
	string[7]  = 'o';
	string[8]  = ' ';
	string[9]  = 'N';
	string[10] = 'o';
	string[11] = 'w';
	string[12] = '!';
	for (int j = 0; j < sizeof(string); j++)
	{
		lcd.setCursor(j,LINE_1); // XXX need to comment here
		lcd.print(string[j]);  
	}
	// XXX need to comment about polimophism technique
	// XXX need to comment about overwrite function technique in print function

	// Xác định Mode khởi động của chương trình
	memset(0, &g_DataBlock, sizeof(g_DataBlock));
	g_DataBlock.curMode = countDownM;
	// Xác định Kiểu 
	g_DataBlock.countDownData.state = stopState;
}

		// XXX should use maxtrix technique to handle instead of create two array
		lcd_buffer_insert(int line, int colum, char *str, int str length, eNumBlink blink);

void loop()
{  
	// Đây là vòng lặp lớn nhất của chương trình, sau khi khởi động xong, MCU sẽ chỉ thực thi
	// trong vòng lặp này mà thôi.

	while (1)
	{
		// Đọc tín hiệu ADC để biết nút nào đã được nhấn
		eButton button = read_buttons(); 

		// Xác định mode hiện tại và thực hiện công việc trong mode đó
		unsigned int modeIndex;
		for (modeIndex = 0; modeIndex < (sizeof(g_Mode) / sizeof(T_Mode)); i++)
		{
			if (g_Mode[modeIndex].mode == g_DataBlock.curMode)
			{
				g_Mode[modeIndex].buttonHandler(button); // xử lý công việc dựa theo nút đã nhấn
				g_Mode[modeIndex].taskHandler();         // xử lý công việc bình thường trong mode
				break;
			}
		}
		if (modeIndex == (sizeof(g_Mode) / sizeof(T_Mode)))
		{
			// Nếu không tìm thấy mode hiện tại trong bảng -> lỗi của chương trình
			// Chương trình chạy tốt sẽ ko bao giờ nhảy vào đoạn này
			lcd.setCursor(j,LINE_1);
			lcd.print("I'm broken!!!");  
		}


		//XXX need to break this file into two file. -> to let they understand about global variable
	}

	// print character to lcd
}
