#include <Windows.h>
#include <stdexcept>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <string>

#define MONOCHROME_MODE		0b010
#define REFRESH_WINDOW_SIZE 0b001

#define DRAW_MODE_ALL		0b111
#define DRAW_MODE_LITERAL	0b001
#define DRAW_MODE_FRONT		0b010
#define DRAW_MODE_BACK		0b100


// ���� ��������
enum Color {
	Black = 0,
	Blue,
	Green,
	Cyan,
	Red,
	Magenta,
	Brown,
	LightGray,
	DarkGray,
	LightBlue,
	LightGreen,
	LightCyan,
	LightRed,
	LightMagenta,
	Yellow,
	White,
};


// ������ ��� 1 ���������� ������
struct Tile {
	wchar_t lit = '-';
	Color front = Color::White;
	Color back = Color::Black;

	// ��������� �� �������, ����� ������� � ����� ����
	inline bool operator==(const Tile& tile) {
		return tile.lit == lit and tile.front == front and tile.back == back;
	}

	// ��������� �� �������, ����� ������� � ����� ����
	inline bool operator!=(const Tile& tile) {
		return tile.lit != lit or tile.front != front or tile.back != back;
	}

	// ���� ���� � ������� � wAttributes
	inline WORD ColorToAttribute() {
		return (back << 4) | front;
	}
};


// ������� ������
class RenderRect {
public:
	// ������� �������
	RenderRect(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
		_x0 = x0;
		_y0 = y0;
		_x1 = x1;
		_y1 = y1;
		_bufer.resize((_x1 - _x0) * (_y1 - _y0));
	}

	// �������� ������
	RenderRect(const RenderRect& screen) {
		_x0 = screen._x0;
		_y0 = screen._y0;
		_x1 = screen._x1;
		_y1 = screen._y1;
		_bufer = screen._bufer;
	}

	// �������� �����������
	void operator=(const RenderRect& screen) {
		_x0 = screen._x0;
		_y0 = screen._y0;
		_x1 = screen._x1;
		_y1 = screen._y1;
		_bufer = screen._bufer;
	}

	// ������ ������ �� ��������� �����������
	// ���� globalCoords = false, �� ���������� �������� ������������ (x0, y0)
	// �������� true � ������ ������
	bool set(int32_t x, int32_t y, const Tile& tile, bool globalCoords = false) {
		if (globalCoords) {
			x -= _x0;
			y -= _y0;
		}

		if (width() <= x or x < 0 or height() <= y or y < 0)
			return false;

		Tile& this_tile = _bufer[y * width() + x];
		if (this_tile != tile)
			this_tile = tile;
		return true;
	}

	// ��������� ������ �� ��������� �����������
	// ���� globalCoords = false, �� ���������� �������� ������������ (x0, y0)
	// � ������ ������� ������ ������ ����� NULL
	Tile get(int32_t x, int32_t y, bool globalCoords = false) const{
		if (globalCoords) {
			x -= _x0;
			y -= _y0;
		}

		if (width() <= x or x < 0 or height() <= y or y < 0)
			return { '\0' };

		return _bufer.at((y * width() + x));
	}

	// ��������� ��� ������ ��������� �������
	inline void fill(const Tile& tile) {
		for (auto& _tile : _bufer)
			if (_tile != tile)
				_tile = tile;
	}

	// ������� �������
	void move(int32_t x, int32_t y) {
		_x0 += x;
		_y0 += y;
		_x1 += x;
		_y1 += y;
	}

	// �������� ���������
	void replace(int32_t x, int32_t y) {
		move(x - _x0, y - _y0);
	}

	// ������ ������ 
	inline size_t length() const {
		return _bufer.size();
	}

	// ������ �������
	inline int32_t width() const {
		return _x1 - _x0;
	}

	// ������ �������
	inline int32_t height() const  {
		return _y1 - _y0;
	}

	const int32_t& x0 = _x0;
	const int32_t& y0 = _y0;
	const int32_t& x1 = _x1;
	const int32_t& y1 = _y1;

private:

	int32_t _x0, _y0, _x1, _y1;	// ���������� ��������������
	std::vector<Tile> _bufer;	// ����� ������

};


// ��� ��� ������ � �������
class Window{
public:
	Window() : 
		_w(0), _h(0), 
		_hConsole(NULL), _hScene(NULL),
		_screen(0, 0, 0, 0) {};

	// ������� ���� ��������� �������
	void create(uint32_t w, uint32_t h, const wchar_t* title) {
		SetConsoleTitle(title);
		_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		createBufer();
		resize(w, h);
	}

	// ������� �������������� ����� �������
	void createBufer() {
		_hScene = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, NULL, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		return;
	}

	// �������� ������� ����
	void resize(uint32_t w, uint32_t h) {
		_w = w;
		_h = h;

		_screen = RenderRect(0, 0, _w + 1, _h + 1);

		COORD size = { _w + 2, _h + 2 };
		SetConsoleScreenBufferSize(_hConsole, size);
		SetConsoleScreenBufferSize(_hScene, size);
		refreshSize();
	}

	// ������� �����
	void clear(const Tile& tile = Tile()) {
		_screen.fill(tile);
	}

	// ��������� RenderRect
	void draw(const RenderRect& screen, int mode = DRAW_MODE_ALL) {
		for (int32_t y = screen.y0; y < screen.y1; y++) {
			for (int32_t x = screen.x0; x < screen.x1; x++) {
				Tile tile = screen.get(x, y, true);
				Tile _tile = _screen.get(x, y, true);

				if (!(mode & DRAW_MODE_LITERAL))
					tile.lit = _tile.lit;
				if (!(mode & DRAW_MODE_FRONT))
					tile.front = _tile.front;
				if (!(mode & DRAW_MODE_BACK))
					tile.back = _tile.back;

				_screen.set(x, y, tile, true);
			}
		}
		return;
	}

	// ������������ �����
	void display(int mode) {
		if (mode & REFRESH_WINDOW_SIZE) 
			refreshSize();

		// ����������� ���������
		if (mode & MONOCHROME_MODE) {
			// �������� �������� � ������ �������
			std::wstring _literals;
			for (int32_t y = 0; y < _screen.height(); y++) {
				for (int32_t x = 0; x < _screen.width(); x++) {
					_literals += _screen.get(x, y).lit;
				}
				_literals += '\n';
			}
			WriteConsole(_hScene, _literals.c_str(), _literals.size(), NULL, NULL);
		}

		// ������� ��������� 
		else {
			// �������� �������� � ������ �������
			std::wstring _literals;
			
			for (int32_t y = 0; y < _screen.height(); y++) {
				for (int32_t x = 0; x < _screen.width(); x++)
					_literals += _screen.get(x, y).lit;
				_literals += '\n';
			}
			WriteConsole(_hScene, _literals.c_str(), _literals.size(), NULL, NULL);


			std::vector<WORD> _attributes(_screen.width());
			for (int32_t y = 0; y < _screen.height(); y++) {
				for (int32_t x = 0; x < _screen.width(); x++)
					_attributes[x] = _screen.get(x, y).ColorToAttribute();
				COORD begin = { 0, y };
				DWORD written = 0;
				auto a = WriteConsoleOutputAttribute(_hScene, &_attributes[0], _attributes.size(), begin, &written);
			}	
		}

		// �������� �� ����� � �������
		std::vector<CHAR_INFO> bufer(_w * _h);
		SMALL_RECT rect = { 0, 0, _w - 1, _h - 1 };
		ReadConsoleOutput(
			_hScene, &bufer[0],
			{ (SHORT)_w, (SHORT)_h },
			{ (SHORT)0, (SHORT)0 },
			&rect);

		WriteConsoleOutput(
			_hConsole, &bufer[0],
			{ (SHORT)_w, (SHORT)_h },
			{ (SHORT)0, (SHORT)0 },
			&rect);
	}
protected:
	// �������������� ������ ����
	void refreshSize() {
		SMALL_RECT rect = { 0, 0, _w + 1, _h + 1};
		SetConsoleWindowInfo(_hConsole, true, &rect);
		SetConsoleWindowInfo(_hScene, true, &rect);
	}

	uint32_t _w, _h;		// ������ � ������ �����
	HANDLE _hConsole;		// ���������� �������
	HANDLE _hScene;			// �����
	RenderRect _screen;		// ����� ����
};