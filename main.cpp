namespace std {
struct string_view {
    int S;
    const char* D;
    constexpr string_view() : S(0), D(0){}
    constexpr string_view(const char* Str) : S(__builtin_strlen(Str)), D(Str) {}
    constexpr string_view(int Size, const char* Str) : S(Size), D(Str) {}
    constexpr int size() const {
        return S;
    }
    constexpr const char* data() const {
        return D;
    }
    constexpr bool empty() const {
      return size() == 0;
    }
};
}

consteval std::string_view hello(std::string_view input) {
	if (input.empty()) {
		throw 1; // BEFORE: compile-time error at throw expression site when reached
	}
	
	return std::string_view("HAHAHA");
}

constexpr std::string_view foo() {
    try {
        const auto c = hello(""); // AFTER: this exception is caught
        return c;
    } catch (...) {
        // everything is fine
        return "";
    }
}

constexpr std::string_view Str = foo();
