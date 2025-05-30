#include "Render.h"
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "GUItextRectangle.h"





#ifdef _DEBUG
#include <Debugapi.h> 
struct debug_print
{
	template<class C>
	debug_print& operator<<(const C& a)
	{
		OutputDebugStringA((std::stringstream() << a).str().c_str());
		return *this;
	}
} debout;
#else
struct debug_print
{
	template<class C>
	debug_print& operator<<(const C& a)
	{
		return *this;
	}
} debout;
#endif

//библиотека для разгрузки изображений
//https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;


bool texturing = true;
bool lightning = true;
bool alpha = false;


//переключение режимов освещения, текстурирования, альфаналожения
void switchModes(OpenGL *sender, KeyEventArg arg)
{
	//конвертируем код клавиши в букву
	auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

	switch (key)
	{
	case 'L':
		lightning = !lightning;
		break;
	case 'T':
		texturing = !texturing;
		break;
	case 'A':
		alpha = !alpha;
		break;
	}
}

//Текстовый прямоугольничек в верхнем правом углу.
//OGL не предоставляет возможности для хранения текста
//внутри этого класса создается картинка с текстом (через виндовый GDI),
//в виде текстуры накладывается на прямоугольник и рисуется на экране.
//Это самый простой способ что то написать на экране
//но ооооочень не оптимальный
GuiTextRectangle text;

//айдишник для текстуры
GLuint texId;
//выполняется один раз перед первым рендером
void initRender()
{
	//==============НАСТРОЙКА ТЕКСТУР================
	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//просим сгенерировать нам Id для текстуры
	//и положить его в texId
	glGenTextures(1, &texId);

	//делаем текущую текстуру активной
	//все, что ниже будет применено texId текстуре.
	glBindTexture(GL_TEXTURE_2D, texId);


	int x, y, n;

	//загружаем картинку
	//см. #include "stb_image.h" 
	unsigned char* data = stbi_load("razvkek.png", &x, &y, &n, 4);
	//unsigned char* data = stbi_load("razv.png", &x, &y, &n, 4);

	//x - ширина изображения
	//y - высота изображения
	//n - количество каналов
	//4 - нужное нам количество каналов
	//пиксели будут хранится в памяти [R-G-B-A]-[R-G-B-A]-[..... 
	// по 4 байта на пиксель - по байту на канал
	//пустые каналы будут равны 255

	//Картинка хранится в памяти перевернутой 
	//так как ее начало в левом верхнем углу
	//по этому мы ее переворачиваем -
	//меняем первую строку с последней,
	//вторую с предпоследней, и.т.д.
	unsigned char* _tmp = new unsigned char[x * 4]; //времянка
	for (int i = 0; i < y / 2; ++i)
	{
		std::memcpy(_tmp, data + i * x * 4, x * 4);//переносим строку i в времянку
		std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4); //(y-1-i)я строка -> iя строка
		std::memcpy(data + (y - 1 - i) * x * 4, _tmp, x * 4); //времянка -> (y-1-i)я строка
	}
	delete[] _tmp;


	//загрузка изображения в видеопамять
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	//выгрузка изображения из опперативной памяти
	stbi_image_free(data);


	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
												  //GL_REPLACE -- полная замена политога текстурой
	//настройка тайлинга
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//настройка фильтрации
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//======================================================

	//================НАСТРОЙКА КАМЕРЫ======================
	camera.caclulateCameraPos();

	//привязываем камеру к событиям "движка"
	gl.WheelEvent.reaction(&camera, &Camera::Zoom);
	gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
	gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
	gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
	gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
	//==============НАСТРОЙКА СВЕТА===========================
	//привязываем свет к событиям "движка"
	gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
	gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
	gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
	//========================================================
	//====================Прочее==============================
	gl.KeyDownEvent.reaction(switchModes);
	text.setSize(512, 180);
	//========================================================
	   

	camera.setPosition(15, 15, 15);
}



#include "Render.h"

#include <math.h>
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <random>
#include <vector>

void figure(int height = 0)
{
	glBegin(GL_QUADS);

	//glVertex3d(2, 9, height);
	//glVertex3d(7, 4, height);
	//glVertex3d(-1, 0, height);
	//glVertex3d(-7, 6, height);
	if (height > 0)
	{
		glNormal3f(0, 0, 1);
		glTexCoord2f(0.159, 1 - 0.237);
		glVertex3d(-1, 0, height);
		glTexCoord2f(0.072, 1 - 0.280);
		glVertex3d(-5, -2, height);
		glTexCoord2f(0.160, 1 - 0.423);
		glVertex3d(-1, -8, height);
		glTexCoord2f(0.280, 1 - 0.400);
		glVertex3d(4, -7, height);

		glNormal3f(0, 0, 1);
		glTexCoord2f(0.159, 1 - 0.237);
		glVertex3d(-1, 0, height);
		glTexCoord2f(0.280, 1 - 0.400);
		glVertex3d(4, -7, height);
		glTexCoord2f(0.211, 1 - 0.237);
		glVertex3d(1, 0, height);
		glTexCoord2f(0.359, 1 - 0.140);
		glVertex3d(7, 4, height);
	}
	else
	{
		glNormal3f(0, 0, -1);
		glTexCoord2f(0.580, 1 - 0.237);
		glVertex3d(-1, 0, height);
		glTexCoord2f(0.466, 1 - 0.400);
		glVertex3d(4, -7, height);
		glTexCoord2f(0.585, 1 - 0.418);
		glVertex3d(-1, -8, height);
		glTexCoord2f(0.675, 1 - 0.280);
		glVertex3d(-5, -2, height);

		glNormal3f(0, 0, -1);
		glTexCoord2f(0.580, 1 - 0.237);
		glVertex3d(-1, 0, height);
		glTexCoord2f(0.388, 1 - 0.140);
		glVertex3d(7, 4, height);
		glTexCoord2f(0.535, 1 - 0.237);
		glVertex3d(1, 0, height);
		glTexCoord2f(0.466, 1 - 0.400);
		glVertex3d(4, -7, height);
	}
	glEnd();
}

void walls(int height = 0)
{
	glBegin(GL_QUADS);

	float n_x, n_y, n_z;
	float n_mod;

	n_x = -5;
	n_y = 1;
	n_z = 0;
	n_mod = sqrt(n_x * n_x + n_y * n_y + n_z * n_z);

	glNormal3f(n_x / n_mod, n_y / n_mod, n_z / n_mod);
	glTexCoord2f(0.469, 1 - 0.456);
	glVertex3d(-1, 0, height);
	glTexCoord2f(0.469, 1 - 0.508);
	glVertex3d(-1, 0, 0);
	glTexCoord2f(0.577, 1 - 0.508);
	glVertex3d(-5, -2, 0); //2
	glTexCoord2f(0.577, 1 - 0.456);
	glVertex3d(-5, -2, height);

	/*

	glVertex3d(-5, -2, 0);
	glVertex3d(-5, -2, height);
	glVertex3d(-1, -8, height);
	glVertex3d(-1, -8, 0);*/

	n_x = 2;
	n_y = -10;
	n_z = 0;
	n_mod = sqrt(n_x * n_x + n_y * n_y + n_z * n_z);
	glNormal3f(n_x / n_mod, n_y / n_mod, n_z / n_mod);
	glTexCoord2f(0.313, 1 - 0.530);
	glVertex3d(-1, -8, height);
	glTexCoord2f(0.313, 1 - 0.583);
	glVertex3d(-1, -8, 0);
	glTexCoord2f(0.437, 1 - 0.583);
	glVertex3d(4, -7, 0);
	glTexCoord2f(0.437, 1 - 0.530);
	glVertex3d(4, -7, height);

	n_x = 14;
	n_y = 6;
	n_z = 0;
	n_mod = sqrt(n_x * n_x + n_y * n_y + n_z * n_z);
	glNormal3f(n_x / n_mod, n_y / n_mod, n_z / n_mod);
	glTexCoord2f(0.010, 1 - 0.508);
	glVertex3d(4, -7, 0);
	glTexCoord2f(0.010, 1 - 0.451);
	glVertex3d(4, -7, height);
	glTexCoord2f(0.217, 1 - 0.451);
	glVertex3d(1, 0, height);
	glTexCoord2f(0.217, 1 - 0.508);
	glVertex3d(1, 0, 0);

	n_x = 8;
	n_y = -12;
	n_z = 0;
	n_mod = sqrt(n_x * n_x + n_y * n_y + n_z * n_z);
	glNormal3f(n_x / n_mod, n_y / n_mod, n_z / n_mod);
	glTexCoord2f(0.236, 1 - 0.451);
	glVertex3d(1, 0, height);
	glTexCoord2f(0.236, 1 - 0.508);
	glVertex3d(1, 0, 0);
	glTexCoord2f(0.443, 1 - 0.508);
	glVertex3d(7, 4, 0);
	glTexCoord2f(0.443, 1 - 0.451);
	glVertex3d(7, 4, height);

	n_x = 10;
	n_y = 10;
	n_z = 0;
	n_mod = sqrt(n_x * n_x + n_y * n_y + n_z * n_z);
	glNormal3f(n_x / n_mod, n_y / n_mod, n_z / n_mod);
	glTexCoord2f(0.674, 1 - 0.530);
	glVertex3d(7, 4, 0);
	glTexCoord2f(0.674, 1 - 0.583);
	glVertex3d(7, 4, height);
	glTexCoord2f(0.848, 1 - 0.583);
	glVertex3d(2, 9, height);
	glTexCoord2f(0.848, 1 - 0.530);
	glVertex3d(2, 9, 0);

	/*glColor3d(r(gen), r(gen), r(gen));

	glVertex3d(2, 9, height);
	glVertex3d(2, 9, 0);
	glVertex3d(-7, 6, 0);
	glVertex3d(-7, 6, height); */

	n_x = -12;
	n_y = -12;
	n_z = 0;
	n_mod = sqrt(n_x * n_x + n_y * n_y + n_z * n_z);
	glNormal3f(n_x / n_mod, n_y / n_mod, n_z / n_mod);
	glTexCoord2f(0.464, 1 - 0.530);
	glVertex3d(-7, 6, 0);
	glTexCoord2f(0.464, 1 - 0.583);
	glVertex3d(-7, 6, height);
	glTexCoord2f(0.650, 1 - 0.583);
	glVertex3d(-1, 0, height);
	glTexCoord2f(0.650, 1 - 0.530);
	glVertex3d(-1, 0, 0);

	glEnd();
}

void DrawHalfCircle(int height = 0)
{
	glBegin(GL_TRIANGLES);
	int number = 50;
	float radius = sqrt(13);
	float pi = 3.14159;
	float phi = pi / 2 + asin(2 / sqrt(13));
	if (height > 0)
	{
		for (int i = 0; i < number; ++i) {
			glNormal3f(0, 0, 1);
			glTexCoord2f(0.116 + 0.0838 * cosf(phi + (i)*pi / number), 1 - 0.350 + 0.0838 * sinf(phi + (i)*pi / number));
			glVertex3f(radius * cosf(i * pi / number), radius * sinf(i * pi / number), height);
			glTexCoord2f(0.116 + 0.0838 * cosf(phi + (i + 1) * pi / number), 1 - 0.350 + 0.0838 * sinf(phi + (i + 1) * pi / number));
			glVertex3f(radius * cosf((i + 1) * pi / number), radius * sinf((i + 1) * pi / number), height);
			glTexCoord2f(0.116, 1 - 0.350);
			glVertex3f(0, 0, height);
		}
	}
	else
	{
		phi = pi + asin(3 / sqrt(13));
		for (int i = 0; i < number; ++i) {
			glNormal3f(0, 0, -1);
			glTexCoord2f(0.627 + 0.0838 * cosf(phi + (number - i)*pi / number), 1 - 0.350 + 0.0838 * sinf(phi + (number - i)*pi / number));
			glVertex3f(radius * cosf(i * pi / number), radius * sinf(i * pi / number), height);
			glTexCoord2f(0.627 + 0.0838 * cosf(phi + (number - i - 1) * pi / number), 1 - 0.350 + 0.0838 * sinf(phi + (number - i - 1) * pi / number));
			glVertex3f(radius * cosf((i + 1) * pi / number), radius * sinf((i + 1) * pi / number), height);
			glTexCoord2f(0.627, 1 - 0.350);
			glVertex3f(0, 0, height);
		}
	}
	glEnd();
}

void DrawWallsCircle(int height = 0)
{
	glBegin(GL_QUADS);
	int number = 50;
	float radius = sqrt(13);
	float pi = 3.14159;
	float x1, x2;
	float y1, y2;
	float n_x, n_y, n_z;
	float n_mod;
	x2 = radius * cos(0);
	y2 = radius * sin(0);
	for (int i = 0; i < number; ++i) {
		x1 = x2;
		x2 = radius * (cosf((i + 1) * pi / number));
		y1 = y2;
		y2 = radius * sinf((i + 1) * pi / number);
		n_x = (y2 - y1) * height;
		n_y = -(x2 - x1) * height;
		n_z = 0;
		n_mod = sqrt(n_x * n_x + n_y * n_y);
		glNormal3f(n_x / n_mod, n_y / n_mod, 0);
		glTexCoord2f(0.011 + (i + 1) * (0.289 - 0.011) / number, 1 - 0.583);
		glVertex3f(x1, y1, 0);
		glTexCoord2f(0.011 + (i + 1) * (0.289 - 0.011) / number, 1 - 0.531);
		glVertex3f(x1, y1, height);
		glTexCoord2f(0.011 + (i + 1) * (0.289 - 0.011) / number, 1 - 0.531);
		glVertex3f(x2, y2, height);
		glTexCoord2f(0.011 + (i + 1) * (0.289 - 0.011) / number, 1 - 0.583);
		glVertex3f(x2, y2, 0);

	}
	glEnd();
}

void DrawWallByCircle(int height = 0)
{
	glBegin(GL_QUADS);
	int number = 50;
	float radius = sqrt(45);
	float rotate = 3.14159 - asin(6 / sqrt(45)) - asin(3 / sqrt(45));
	float angle = asin(6 / sqrt(45)) + 3.14159;
	float phi = 3.14159 + asin(2 / sqrt(5));
	if (height > 0)
	{
		
		for (int i = 0; i < number; ++i) {
			glNormal3f(0, 0, 1);
			glTexCoord2f(0.090 + 0.168 * cosf(phi + i * rotate / number), 1 + 0.060 + 0.168 * sinf(phi + i * rotate / number));
			glVertex3f(radius * (cosf(i * rotate / number)), radius * sinf(i * rotate / number), height);
			glTexCoord2f(0.090 + 0.168 * cosf(phi + (i + 1) * rotate / number), 1 + 0.060 + 0.168 * sinf(phi + (i + 1) * rotate / number));
			glVertex3f(radius * (cosf((i + 1) * rotate / number)), radius * sinf((i + 1) * rotate / number), height);
			glTexCoord2f(0.159 + 0.200 * (i + 1) / number, 1 - 0.237 + 0.097 * (i + 1) / number);
			glVertex3f((3 + 8 * (i + 1) / sqrt(number * number)) * cosf(angle) + (-12 + 4 * (i + 1) / sqrt(number * number)) * sinf(angle), -(3 + 8 * (i + 1) / sqrt(number * number)) * sinf(angle) + (-12 + 4 * (i + 1) / sqrt(number * number)) * cosf(angle), height);
			glTexCoord2f(0.159 + 0.200 * (i) / number, 1 - 0.237 + 0.097 * (i) / number);
			glVertex3f((3 + 8 * i / sqrt(number * number)) * cosf(angle) + (-12 + 4 * i / sqrt(number * number)) * sinf(angle), -(3 + 8 * i / sqrt(number * number)) * sinf(angle) + (-12 + 4 * i / sqrt(number * number)) * cosf(angle), height);

		}
	}
	else
	{
		for (int i = 0; i < number; ++i) {
			float phi = 3.14159 + asin(1 / sqrt(5));
			glNormal3f(0, 0, -1);
			glTexCoord2f(0.654 + 0.168 * cosf(phi + (number - i) * rotate / number), 1 + 0.060 + 0.168 * sinf(phi + (number - i) * rotate / number));
			glVertex3f(radius * (cosf(i * rotate / number)), radius * sinf(i * rotate / number), height);
			glTexCoord2f(0.654 + 0.168 * cosf(phi + (number - i - 1) * rotate / number), 1 + 0.060 + 0.168 * sinf(phi + (number - i - 1) * rotate / number));
			glVertex3f(radius * (cosf((i + 1) * rotate / number)), radius * sinf((i + 1) * rotate / number), height);
			glTexCoord2f(0.585 - 0.200 * (i + 1) / number, 1 - 0.237 + 0.097 * (i + 1) / number);
			glVertex3f((3 + 8 * (i + 1) / sqrt(number * number)) * cosf(angle) + (-12 + 4 * (i + 1) / sqrt(number * number)) * sinf(angle), -(3 + 8 * (i + 1) / sqrt(number * number)) * sinf(angle) + (-12 + 4 * (i + 1) / sqrt(number * number)) * cosf(angle), height);
			glTexCoord2f(0.585 - 0.200 * (i) / number, 1 - 0.237 + 0.097 * (i) / number);
			glVertex3f((3 + 8 * i / sqrt(number * number)) * cosf(angle) + (-12 + 4 * i / sqrt(number * number)) * sinf(angle), -(3 + 8 * i / sqrt(number * number)) * sinf(angle) + (-12 + 4 * i / sqrt(number * number)) * cosf(angle), height);

		}
	}
	glEnd();
}

void DrawWallWallByCircle(int height)
{
	glBegin(GL_QUADS);
	int number = 50;
	float radius = sqrt(45);
	float rotate = 3.14159 - asin(6 / sqrt(45)) - asin(3 / sqrt(45));
	float angle = asin(6 / sqrt(45)) + 3.14159;
	float x1, x2;
	float y1, y2;
	float n_x, n_y, n_z;
	float n_mod;
	x2 = radius * cos(0);
	y2 = radius * sin(0);
	for (int i = 0; i < number; ++i) {
		x1 = x2;
		x2 = radius * (cosf((i + 1) * rotate / number));
		y1 = y2;
		y2 = radius * sinf((i + 1) * rotate / number);
		n_x = -(y2 - y1) * height;
		n_y = (x2 - x1) * height;
		n_z = 0;
		n_mod = sqrt(n_x * n_x + n_y * n_y);
		glNormal3f(n_x / n_mod, n_y / n_mod, 0);
		glTexCoord2f(0.009 + i * (0.287 - 0.009) / number, 1 - 0.656);
		glVertex3f(x1, y1, 0);
		glTexCoord2f(0.009 + i * (0.287 - 0.009) / number, 1 - 0.604);
		glVertex3f(x1, y1, height);
		glTexCoord2f(0.009 + (i + 1) * (0.287 - 0.009) / number, 1 - 0.604);
		glVertex3f(x2, y2, height);
		glTexCoord2f(0.009 + (i + 1) * (0.287 - 0.009) / number, 1 - 0.656);
		glVertex3f(x2, y2, 0);
	}
	glEnd();
}

void Drawing50(int height = 0)
{
	glPushMatrix();
	figure(0);
	walls(height);
	figure(height);


	glTranslated(-3, -5, 0);
	glRotated(asin(2 / sqrt(13)) * 180 / 3.14159 + 90, 0, 0, 1);

	DrawWallsCircle(height);

	DrawHalfCircle();

	DrawHalfCircle(height);
	glPopMatrix();
	glPushMatrix();
	glTranslated(-4, 12, 0);
	glRotated(asin(6 / sqrt(45)) * 180 / 3.14159 + 180, 0, 0, 1);

	DrawWallByCircle();
	DrawWallByCircle(height);

	DrawWallWallByCircle(height);

	glPopMatrix();
};


void Render(double delta_time)
{    
	glEnable(GL_DEPTH_TEST);
	
	//натройка камеры и света
	//в этих функциях находятся OGLные функции
	//которые устанавливают параметры источника света
	//и моделвью матрицу, связанные с камерой.

	if (gl.isKeyPressed('F')) //если нажата F - свет из камеры
	{
		light.SetPosition(camera.x(), camera.y(), camera.z());
	}
	camera.SetUpCamera();
	light.SetUpLight();


	//рисуем оси
	gl.DrawAxes();

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	

	//включаем режимы, в зависимости от нажания клавиш. см void switchModes(OpenGL *sender, KeyEventArg arg)
	if (lightning)
		glEnable(GL_LIGHTING);
	if (texturing)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0); //сбрасываем текущую текстуру
	}
		
	if (alpha)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
		
	//=============НАСТРОЙКА МАТЕРИАЛА==============


	//настройка материала, все что рисуется ниже будет иметь этот метериал.
	//массивы с настройками материала
	float  amb[] = { 0.2, 0.2, 0.1, 1. };
	float dif[] = { 0.4, 0.65, 0.5, 1. };
	float spec[] = { 0.9, 0.8, 0.3, 1. };
	float sh = 0.2f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); 
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH); //закраска по Гуро      
			   //(GL_SMOOTH - плоская закраска)

	//============ РИСОВАТЬ ТУТ ==============
	glBindTexture(GL_TEXTURE_2D, texId); //================================================================================код тут=================================
	Drawing50(2);

	//квадратик станкина
	//так как расчет освещения происходит только в вершинах
	// (закраска по Гуро)
	//то рисуем квадратик из более маленьких квадратиков
	glBindTexture(GL_TEXTURE_2D, texId);


	//===============================================

	//рисуем источник света
	light.DrawLightGizmo();

	//================Сообщение в верхнем левом углу=======================

	//переключаемся на матрицу проекции
	glMatrixMode(GL_PROJECTION);
	//сохраняем текущую матрицу проекции с перспективным преобразованием
	glPushMatrix();
	//загружаем единичную матрицу в матрицу проекции
	glLoadIdentity();

	//устанавливаем матрицу паралельной проекции
	glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

	//переключаемся на моделвью матрицу
	glMatrixMode(GL_MODELVIEW);
	//сохраняем матрицу
	glPushMatrix();
    //сбразываем все трансформации и настройки камеры загрузкой единичной матрицы
	glLoadIdentity();

	//отрисованное тут будет визуалзироватся в 2д системе координат
	//нижний левый угол окна - точка (0,0)
	//верхний правый угол (ширина_окна - 1, высота_окна - 1)
	
	std::wstringstream ss;
	ss << std::fixed << std::setprecision(3);
	ss << "T - " << (texturing ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"текстур" << std::endl;
	ss << "L - " << (lightning ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"освещение" << std::endl;
	ss << "A - " << (alpha ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"альфа-наложение" << std::endl;
	ss << L"F - Свет из камеры" << std::endl;
	ss << L"G - двигать свет по горизонтали" << std::endl;
	ss << L"G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << L"Коорд. света: (" << std::setw(7) <<  light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7) << light.z() << ")" << std::endl;
	ss << L"Коорд. камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << "," << std::setw(7) << camera.z() << ")" << std::endl;
	ss << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ",fi1=" << std::setw(7) << camera.fi1() << ",fi2=" << std::setw(7) << camera.fi2() << std::endl;
	ss << L"delta_time: " << std::setprecision(5)<< delta_time << std::endl;
	
	text.setPosition(10, gl.getHeight() - 10 - 180);
	text.setText(ss.str().c_str());
	text.Draw();

	//восстанавливаем матрицу проекции на перспективу, которую сохраняли ранее.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}   



