#include <chrono>


class Time {
public:
	// конструктор по умолчанию
	Time() : _time(0) {};

	// конструктор копирования
	Time(const Time& time) {
		_time = time._time;
	}

	// оператор копирования
	void operator=(const Time& time) {
		_time = time._time;
	}

	// время в микросекундах
	inline uint64_t asMicroseconds() {
		return _time.count();
	}

	// время в миллисекундах
	inline uint64_t asMilliseconds() {
		return std::chrono::duration_cast<milliseconds_t>(_time).count();
	}

	// время в секундах
	inline uint64_t asSeconds() {
		return std::chrono::duration_cast<seconds_t>(_time).count();
	}

protected:
	using clock_t = std::chrono::high_resolution_clock;
	using time_point_t = std::chrono::time_point<clock_t>;
	using microseconds_t = std::chrono::microseconds;
	using milliseconds_t = std::chrono::milliseconds;
	using seconds_t = std::chrono::seconds;
	
	// создает экземпляр Time из задержки в микросекундах
	Time makeTime(microseconds_t time) {
		Time tmp_time;
		tmp_time._time = time;
		return tmp_time;
	}

	// временной интервал
	microseconds_t _time;		
};


// класс часов
class Clock: protected Time {
public:
	// конструктор по умолчанию
	Clock() : _time_point(microseconds_t(0)) {};

	// сбрасывает часы
	void restart() {
		this->_time_point = clock_t::now();
	}

	// возращает прощедшее время
	Time elapced() {
		_time = std::chrono::duration_cast<microseconds_t>(clock_t::now() - _time_point);
		return makeTime(_time);
	}

private:
	// время
	time_point_t _time_point;
};