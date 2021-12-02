#ifndef CONSOLE_GRAPHICS_LIBRARY
#define CONSOLE_GRAPHICS_LIBRARY

#include <Windows.h>
#include <memory>
#include <vector>
#include <string>



#ifndef TITLE_BUFFER_SIZE
#define TITLE_BUFFER_SIZE 128
#endif // !TILE_BUFFER_SIZE

#ifndef DEFAULT_TILE_CHARACTER
#define DEFAULT_TILE_CHARACTER L'⍰'
#endif // !DEFAULT_TILE_CHARACTER

#ifndef DEFAULT_TILE_FRONT
#define DEFAULT_TILE_FRONT 0xf
#endif // !DEFAULT_TILE_FRONT

#ifndef DEFAULT_TILE_BACK
#define DEFAULT_TILE_BACK 0x0
#endif // !DEFAULT_TILE_BACK



namespace Color {
	const WORD Black           = 0x0;
	const WORD Blue            = 0x1;
	const WORD Green           = 0x2;
	const WORD Cyan            = 0x3;
	const WORD Red             = 0x4;
	const WORD Magenta         = 0x5;
	const WORD Brown           = 0x6;
	const WORD LightGray       = 0x7;
	const WORD DarkGray        = 0x8;
	const WORD LightBlue       = 0x9;
	const WORD LightGreen      = 0xa;
	const WORD LightCyan       = 0xb;
	const WORD LightRed        = 0xc;
	const WORD LightMagenta    = 0xd;
	const WORD Yellow          = 0xe;
	const WORD White           = 0xf;
};



class Tile : public CHAR_INFO {
public:
	Tile() : Tile(DEFAULT_TILE_CHARACTER, DEFAULT_TILE_FRONT, DEFAULT_TILE_BACK) {};

	Tile(WCHAR character): Tile(character, DEFAULT_TILE_FRONT, DEFAULT_TILE_BACK) {};

	Tile(WCHAR character, WORD front, WORD back) {
		this->Char.UnicodeChar = character;
		this->Attributes = (back << 4) | front;
	}

	inline void set_character(WCHAR character) {
		this->Char.UnicodeChar = character;
	}
	
	inline WCHAR get_character() const{
		return this->Char.UnicodeChar;
	}

	inline void set_front(WORD front) {
		this->Attributes = (this->Attributes & 0xf0) | front;
	}

	inline WORD get_front() const {
		return this->Attributes & 0x0f;
	}

	inline void set_back(WORD back) {
		this->Attributes = (this->Attributes & 0x0f) | (back << 4);
	}

	inline WORD get_back() const {
		return (this->Attributes & 0xf0) >> 4;
	}
};



class Surface {
public:
	Surface() : _buf{ NULL }, _size{ 0, 0 }, _position{ 0,0 } {};

	Surface(COORD size, Tile fill = Tile()) {
		_size = size;
		_position = { 0,0 };
		_buf = std::make_shared<TileBuffer>(TileBuffer(_size.X * _size.Y, fill));
	}

	Surface(const Surface& surface) {
		_size = surface._size;
		_position = surface._position;
		_buf = std::shared_ptr<TileBuffer>(surface._buf);
	}

	inline Surface copy() const {
		Surface copy;
		copy._size = _size;
		copy._position = _position;
		copy._buf = std::make_shared<TileBuffer>(TileBuffer());
		copy._buf->insert(copy._buf->begin(), _buf->begin(), _buf->end());
		return copy;
	}

	inline void set_tile(COORD coord, Tile tile) {
		_buf->at(coord.Y * _size.X + coord.X) = tile;
	}

	inline Tile get_tile(COORD coord) const {
		return _buf->at(coord.Y * _size.X + coord.X);
	}

	inline void set_position(COORD position) {
		_position = position;
	}

	inline COORD get_position() const {
		return _position;
	}

	inline const Tile* raw() const {
		return &_buf->front();
	}

	inline SMALL_RECT region(BOOL global = false) const {
		if (global) {
			return {
				_position.X,
				_position.Y,
				static_cast<SHORT>(_position.X + _size.X),
				static_cast<SHORT>(_position.Y + _size.Y)
			};
		}
		else return { 0, 0, _size.X, _size.Y };
	}

	inline COORD size() const{
		return _size;
	}

	inline void fill(Tile tile) {
		return std::fill(_buf->begin(), _buf->end(), tile);
	}

	inline COORD move(COORD shift) {
		_position.X += shift.X;
		_position.Y += shift.Y;
		return get_position();
	}

	inline void blit(const Surface& surface, COORD position, const SMALL_RECT& region) {
		for (SHORT i = 0; i < (region.Right - region.Left) and (position.X + i) < _size.X; ++i) {
			for (SHORT j = 0; j < (region.Bottom - region.Top) and (position.Y + j) < _size.Y; ++j) {
				set_tile(
					{
						static_cast<SHORT>(position.X + i),
						static_cast<SHORT>(position.Y + j)
					},
					surface.get_tile(
						{ static_cast<SHORT>(i + region.Left),
							static_cast<SHORT>(j + region.Top)
						}
					)
				);
			}
		}
	}

	inline Surface crop(const SMALL_RECT& region) const{
		Surface fragment(
			{
				static_cast<SHORT>(region.Right - region.Left),
				static_cast<SHORT>(region.Bottom - region.Top)
			}
		);

		fragment.blit(*this, { 0, 0 }, region);
		return fragment;
	}

private:
	using TileBuffer = std::vector<Tile>;

	std::shared_ptr<TileBuffer> _buf;
	COORD _size;
	COORD _position;
};



class Console : public Surface {
public:
	Console(COORD size) : Surface(size),
		_hwnd{ GetConsoleWindow() },
		_hConsole{ GetStdHandle(STD_OUTPUT_HANDLE) },
		_region{ Surface::region() },
		_title{ NULL },
		_characterBuf{ std::wstring(size.X * size.Y, DEFAULT_TILE_CHARACTER) }
	{
		SetWindowLong(
			_hwnd,
			GWL_STYLE,
			WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX & ~WS_THICKFRAME
		);

		DWORD mods;
		GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mods);
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mods & ~ENABLE_QUICK_EDIT_MODE);

		auto a = SetConsoleWindowInfo(
			_hConsole,
			true,
			region()
		);
	}

	void display(BOOL only_characters = false) {
		if (only_characters) {
			for (INT i = 0; i < size().X * size().Y; ++i) {
				_characterBuf[i] = raw()[i].get_character();
			}
			WriteConsole(
				_hConsole,
				_characterBuf.c_str(),
				size().X * size().Y,
				NULL,
				NULL
			);
			SetConsoleCursorPosition(_hConsole, { 0, 0 });
		}
		else WriteConsoleOutput(
			_hConsole,
			raw(),
			size(),
			{ 0, 0 },
			&_region
		);
	};

	const SMALL_RECT* region() const {
		return &_region;
	}

	void set_title(WCHAR* title){
		for (INT i = 0; i < TITLE_BUFFER_SIZE; ++i) {
			_title[i] = title[i];
			if (title[i] == NULL) break;
		}
		
		SetConsoleTitle(_title);
	}

	WCHAR* get_title() const {
		return (WCHAR*)_title;
	}

	static inline BOOL set_focus(const SMALL_RECT* rect) {
		SetConsoleScreenBufferSize(
			GetStdHandle(STD_OUTPUT_HANDLE),
			{
				static_cast<SHORT>(rect->Right - rect->Left - 1),
				static_cast<SHORT>(rect->Bottom - rect->Top - 1)
			}
		);

		return SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, rect);
	}

private:

	HWND _hwnd;
	HANDLE _hConsole;
	SMALL_RECT _region;
	WCHAR _title[TITLE_BUFFER_SIZE + 1];
	std::wstring _characterBuf;
};

#endif // !CONSOLE_GRAPHICS_LIBRARY