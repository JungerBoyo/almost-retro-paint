#include <memory>
#include <cinttypes>
#include <vector>
#include <array>
#include <filesystem>
#include <iostream>

#include <CLI/CLI.hpp>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <appconfig.hpp>

enum class Tool { CLIXEL, LINE, CIRCLE, ELLIPSE, RECTANGLE };
enum class FillMode { FILLED, EMPTY };
enum class CharMode { BRAILLE, BLOCK };

struct Figure {
	Tool selected_tool;
	CharMode selected_char_mode;
	FillMode selected_fill_mode;
	int x0;
	int y0;
	int x1;
	int y1;
	int r;
	int g;
	int b;
};

std::vector<Figure> figures;

struct DrawAction {
private:
	int start_x_;
	int start_y_;
	std::function<void(ftxui::Canvas&,int,int,int,int)> callback_;

public:
	DrawAction(
		int start_x, 
		int start_y, 
		const std::function<void(ftxui::Canvas&,int,int,int,int)>& drawCallback) 
	 : start_x_(start_x), start_y_(start_y), callback_(drawCallback) {}
	auto startx() const { return start_x_; }
	auto starty() const { return start_y_; }

	void drawCallback(ftxui::Canvas& canvas, int end_x, int end_y) {
		callback_(canvas, start_x_, start_y_, end_x, end_y);
	}
};

std::optional<DrawAction> getDrawAction(
	int m_pos_x,
	int m_pos_y,
	Tool selected_tool,
	CharMode selected_char_mode,
	FillMode selected_fill_mode,
	const std::function<void(ftxui::Pixel&)>& stylizer
) {
	switch (selected_tool) {
	case Tool::LINE: return selected_char_mode == CharMode::BRAILLE ?
		DrawAction(m_pos_x, m_pos_y, 
		[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
			canvas.DrawPointLine(start_x, start_y, end_x, end_y, stylizer);
		}) :
		DrawAction(m_pos_x, m_pos_y, 
		[selected_tool, selected_fill_mode, selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
			canvas.DrawBlockLine(start_x, start_y, end_x, end_y, stylizer);
		});
	case Tool::CIRCLE:  
		if (selected_char_mode == CharMode::BRAILLE) {
			return selected_fill_mode == FillMode::FILLED ?
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					const int x_len = end_x - start_x;
					const int y_len = end_y - start_y;
					const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
					canvas.DrawPointCircleFilled(start_x, start_y, len, stylizer);
				}) :
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					const int x_len = end_x - start_x;
					const int y_len = end_y - start_y;
					const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
												canvas.DrawPointCircle(start_x, start_y, len, stylizer);
				});
		} else {
			return selected_fill_mode == FillMode::FILLED ?
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					const int x_len = end_x - start_x;
					const int y_len = end_y - start_y;
					const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
					canvas.DrawBlockCircleFilled(start_x, start_y, len, stylizer);
				}) :
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					const int x_len = end_x - start_x;
					const int y_len = end_y - start_y;
					const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
					canvas.DrawBlockCircle(start_x, start_y, len, stylizer);
				});
		}
	case Tool::RECTANGLE:
		return selected_char_mode != CharMode::BRAILLE ?
			DrawAction(m_pos_x, m_pos_y, 
			[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
				canvas.DrawBlockLine(start_x, start_y, start_x, end_y, stylizer);
				canvas.DrawBlockLine(start_x, end_y, end_x, end_y, stylizer);
				canvas.DrawBlockLine(end_x, end_y, end_x, start_y, stylizer);
				canvas.DrawBlockLine(end_x, start_y, start_x, start_y, stylizer);
			}) :
			DrawAction(m_pos_x, m_pos_y, 
			[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
				canvas.DrawPointLine(start_x, start_y, start_x, end_y, stylizer);
				canvas.DrawPointLine(start_x, end_y, end_x, end_y, stylizer);
				canvas.DrawPointLine(end_x, end_y, end_x, start_y, stylizer);
				canvas.DrawPointLine(end_x, start_y, start_x, start_y, stylizer);
			});
	case Tool::ELLIPSE: 
		if (selected_char_mode == CharMode::BRAILLE) {
			return selected_fill_mode == FillMode::FILLED ?
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					canvas.DrawPointEllipseFilled(start_x, start_y, end_x, end_y, stylizer);
				}) :
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					canvas.DrawPointEllipse(start_x, start_y, end_x, end_y, stylizer);
				});
		} else {
			return selected_fill_mode == FillMode::FILLED ?
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					canvas.DrawBlockEllipseFilled(start_x, start_y, end_x, end_y, stylizer);
				}) :
				DrawAction(m_pos_x, m_pos_y, 
				[selected_char_mode, stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
					canvas.DrawBlockEllipse(start_x, start_y, end_x, end_y, stylizer);
				});
		}
	default: return std::nullopt;
	}
}

void save(const std::string& file_name, int width, int height) {
	const auto file_path = std::filesystem::current_path() / fmt::format("{}.json", file_name);

	nlohmann::json json_main;
	json_main["width"] = width;
	json_main["height"] = height;
	nlohmann::json json_figures;
	for (const auto figure : figures) {
		nlohmann::json json_figure;
		json_figure["selected_tool"] = static_cast<int>(figure.selected_tool);
		json_figure["selected_char_mode"] = static_cast<int>(figure.selected_char_mode);
		json_figure["selected_fill_mode"] = static_cast<int>(figure.selected_fill_mode);
		json_figure["x0"] = figure.x0;
		json_figure["y0"] = figure.y0;
		json_figure["x1"] = figure.x1;
		json_figure["y1"] = figure.y1;
		json_figure["r"] = figure.r;
		json_figure["g"] = figure.g;
		json_figure["b"] = figure.b;
		json_figures.push_back(json_figure);
	}
	json_main["figures"] = json_figures;

	const auto data = json_main.dump(4);

	std::ofstream stream(file_path);

	stream.write(data.c_str(), data.length());

	stream.close();
}

auto load(const std::string& file_name) {
	const auto file_path = std::filesystem::current_path() / fmt::format("{}.json", file_name);

	std::ifstream stream(file_path);
	nlohmann::json j;
	stream >> j;
	stream.close();

	const auto width = j["width"].get<int>();
	const auto height = j["height"].get<int>();
	const auto json_figures = j["figures"];
	for (const auto& json_figure : json_figures) {
		Figure figure {
			.selected_tool = static_cast<Tool>(json_figure["selected_tool"].get<int>()),
			.selected_char_mode = static_cast<CharMode>(json_figure["selected_char_mode"].get<int>()),
			.selected_fill_mode = static_cast<FillMode>(json_figure["selected_fill_mode"].get<int>()),
			.x0 = json_figure["x0"].get<int>(),
			.y0 = json_figure["y0"].get<int>(),
			.x1 = json_figure["x1"].get<int>(),
			.y1 = json_figure["y1"].get<int>(),
			.r = json_figure["r"].get<int>(),
			.g = json_figure["g"].get<int>(),
			.b = json_figure["b"].get<int>()
		};
		figures.push_back(figure);
	}
	
	return std::make_pair(width, height);
}

void run(const std::string& file_name, int width, int height) {
	std::optional<DrawAction> current_draw_action{std::nullopt};

	// foreground colors	
	int r = 0;
	int g = 0;
	int b = 0;

	auto scr = ftxui::ScreenInteractive::TerminalOutput();
	////												////	
	////				 UI DEFINITION					////
	////												////
	constexpr int mouse_x_position_offset{ 21 };
	constexpr int mouse_y_position_offset{ 1 };
	/*
			canvas component
	*/
	std::string mouse_position_info = "[ 0, 0 ]";
	std::string mouse_motion_info = "Released";
	auto canvas = ftxui::Canvas(width, height);
	auto canvas_copy = ftxui::Canvas(width, height);
	auto canvas_renderer = ftxui::Renderer([&]{
		return ftxui::window(ftxui::text(file_name), ftxui::canvas(canvas));
	}) ;
 
	/*
			lhs ui panel components
	*/
	/// char mode select definition	
	const std::vector<std::string> char_modes{
		"braille dot(⠂)",  // 0 
		"block(█) "        // 1
	};
	int rbox_char_mode_selected{ 0 };
	auto rbox_char_modes = ftxui::Radiobox(&char_modes, &rbox_char_mode_selected);
	/// stylizer checkbox def
	enum class Style { BOLD, BLINK, DIM, UNDERLINED, INVERTED };
	std::vector<std::pair<const std::string, bool>> styles {
		{"bold", false},
		{"blink", false}, 
		{"dim", false}, 
		{"underlined", false}, 
		{"inverted", false}
	};
	auto cboxes_style_container = ftxui::Container::Vertical({});
	for (auto& style : styles) {
		cboxes_style_container->Add(ftxui::Checkbox(style.first, &style.second));
	}
	/// tool definition
	const std::vector<std::string> tools{
		"clixel", // 0
		"line",   // 1
		"circle", // 2
		"elipse", // 3
		"rectangle" // 4
	};
	int rbox_tool_selected{ 0 };
	auto rbox_tools = ftxui::Radiobox(&tools, &rbox_tool_selected);
	/// fill mode
	const std::vector<std::string> fill_modes {
		"filled", // 0
		"empty",  // 1
	};
	int rbox_fill_mode_selected{ 0 };
	auto rbox_fill_modes = ftxui::Radiobox(&fill_modes, &rbox_fill_mode_selected);
	/// clear canvas button 
	const std::string button_clear_canvas_text = "      Clear";
	auto button_clear_canvas = ftxui::Button(button_clear_canvas_text, [&]{
		canvas = ftxui::Canvas(width, height);
		figures.clear();
	});
	/// save canvas button
	const std::string button_save_canvas_text = "      Save";
	auto button_save_canvas = ftxui::Button(button_save_canvas_text, [&]{
		save(file_name, width, height);
	});
	
	/// container
	auto lhs_panel_container = ftxui::Container::Vertical({
		rbox_char_modes,
		cboxes_style_container,
		rbox_fill_modes,
		rbox_tools,
		button_clear_canvas,
		button_save_canvas,
	});
	auto stylizer = [&styles, &r, &g, &b](ftxui::Pixel& p) {
		p.bold = styles[static_cast<std::size_t>(Style::BOLD)].second;
		p.blink = styles[static_cast<std::size_t>(Style::BLINK)].second;
		p.dim = styles[static_cast<std::size_t>(Style::DIM)].second;
		p.underlined = styles[static_cast<std::size_t>(Style::UNDERLINED)].second;
		p.inverted = styles[static_cast<std::size_t>(Style::INVERTED)].second;
		p.foreground_color = ftxui::Color::RGB(static_cast<std::uint8_t>(r), 
												static_cast<std::uint8_t>(g), 
												static_cast<std::uint8_t>(b));
	};
	/*
			rhs ui panel components
	*/
	const std::string button_close_text = "      [ X ]";
	auto button_close = ftxui::Button(button_close_text, scr.ExitLoopClosure());
	/// color picker
	auto slider_r = ftxui::Slider("R:", &r, 0, 255, 1);
	auto slider_g = ftxui::Slider("G:", &g, 0, 255, 1);
	auto slider_b = ftxui::Slider("B:", &b, 0, 255, 1);
	const auto make_color_tile = [&r, &g, &b]{
		return ftxui::text("") |
			   ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 16) |
			   ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN, 8) |
			   ftxui::bgcolor(ftxui::Color::RGB(static_cast<std::uint8_t>(r), 
			   									static_cast<std::uint8_t>(g), 
												static_cast<std::uint8_t>(b)));
	};
	constexpr std::array<char, 16> hex_digits{{
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	}};
	const auto make_color_string = [&r, &g, &b, &hex_digits]{
		return ftxui::text(fmt::format("#0x{}{}{}{}{}{}",
			hex_digits[static_cast<std::size_t>(r / 16)], hex_digits[static_cast<std::size_t>(r % 16)],
			hex_digits[static_cast<std::size_t>(g / 16)], hex_digits[static_cast<std::size_t>(g % 16)],
			hex_digits[static_cast<std::size_t>(b / 16)], hex_digits[static_cast<std::size_t>(b % 16)]
		));
	};

	std::string px0_str, py0_str, px1_str, py1_str;
	// ftxui::InputOption input_option;
	// input_option.on_change = true;
	auto px0_input = ftxui::Input(&px0_str, "0");
	auto py0_input = ftxui::Input(&py0_str, "0");
	auto px1_input = ftxui::Input(&px1_str, "0");
	auto py1_input = ftxui::Input(&py1_str, "0");

	auto button_draw = ftxui::Button("Draw", [&](){
		int x0{0}, y0{0}, x1{0}, y1{0};
		std::from_chars(px0_str.data(), px0_str.data() + px0_str.length(), x0);
		std::from_chars(py0_str.data(), py0_str.data() + py0_str.length(), y0);
		std::from_chars(px1_str.data(), px1_str.data() + px1_str.length(), x1);
		std::from_chars(py1_str.data(), py1_str.data() + py1_str.length(), y1);

		const auto selected_tool = static_cast<Tool>(rbox_tool_selected);
		const auto selected_char_mode = static_cast<CharMode>(rbox_char_mode_selected);
		const auto selected_fill_mode = static_cast<FillMode>(rbox_fill_mode_selected);

		auto draw_action = getDrawAction(x0, y0, selected_tool, selected_char_mode, selected_fill_mode, stylizer);
		if (draw_action.has_value()) {
			draw_action->drawCallback(canvas, x1, y1);
			figures.emplace_back(selected_tool, selected_char_mode, selected_fill_mode, x0, y0, x1, y1, r, g, b);
		}
	});

	if (!figures.empty()) {
		for (const auto figure : figures) {
			r = figure.r;
			g = figure.g;
			b = figure.b;
			auto draw_action = getDrawAction(
				figure.x0, 
				figure.y0, 
				figure.selected_tool, 
				figure.selected_char_mode, 
				figure.selected_fill_mode, 
				stylizer
			);
			if (draw_action.has_value()) {
				draw_action->drawCallback(
					canvas, 
					figure.x1, 
					figure.y1
				);
			}
		}
		r = g = b = 0;
	}


	auto sliders_container = ftxui::Container::Vertical({
		slider_r, slider_g, slider_b	
	});
	auto p0_inputs_container = ftxui::Container::Horizontal({ px0_input, py0_input });
	auto p1_inputs_container = ftxui::Container::Horizontal({ px1_input, py1_input });
	auto inputs_container = ftxui::Container::Vertical({
		p0_inputs_container,
		p1_inputs_container,
		button_draw
	});
	auto rhs_panel_container = ftxui::Container::Vertical({
		button_close,
		sliders_container,
		inputs_container
	});
	/*
			Organize app panel into container
	*/
	auto app_ui_container = ftxui::Container::Horizontal({
		lhs_panel_container,
		canvas_renderer,
		rhs_panel_container
	});

	int prev_m_pos_x{ 0 };
	int prev_m_pos_y{ 0 };

	const auto draw = [&] {
		return ftxui::hbox({
			ftxui::window(ftxui::text(""), ftxui::vbox({
				ftxui::window(ftxui::text("char"), rbox_char_modes->Render()),
				ftxui::window(ftxui::text("styles"), cboxes_style_container->Render()),
				ftxui::window(ftxui::text("fill mode"), rbox_fill_modes->Render()),
				ftxui::window(ftxui::text("tool"), rbox_tools->Render()),
				button_clear_canvas->Render(), 
				ftxui::separator(),
				button_save_canvas->Render(),
				ftxui::filler(),
				ftxui::text(mouse_position_info) | ftxui::center,
				ftxui::text(mouse_motion_info) | ftxui::center
			})),
			ftxui::vbox({canvas_renderer->Render(), ftxui::filler()}),
			ftxui::window(ftxui::text(""), ftxui::vbox({
				button_close->Render(),
				ftxui::separator(),
				ftxui::window(ftxui::text("color picker"), ftxui::vbox({
					make_color_tile(),
					ftxui::separator(),
					slider_r->Render(),
					slider_g->Render(),
					slider_b->Render(),
					ftxui::separator(),
					make_color_string()
				})),
				ftxui::separator(),
				ftxui::hbox({ px0_input->Render(), py0_input->Render() }),
				ftxui::hbox({ px1_input->Render(), py1_input->Render() }),
				button_draw->Render() | ftxui::center
			}))
		});
	};


	auto renderer = ftxui::Renderer(app_ui_container, draw) |
	ftxui::CatchEvent([&](ftxui::Event ev) {
		constexpr auto result{ false };
		auto& mouse = ev.mouse();
		if (ev.is_mouse()) {
			auto m_pos_x = mouse.x - mouse_x_position_offset;
			auto m_pos_y = mouse.y - mouse_y_position_offset;
			mouse_position_info = fmt::format("[ {}, {} ]", m_pos_x, m_pos_y);
			if (mouse.button == ftxui::Mouse::Button::Left) {
				if (mouse.motion == ftxui::Mouse::Motion::Pressed &&
					m_pos_x > 0 && m_pos_x < width/2 &&
					m_pos_y > 0 && m_pos_y < height/4) {
					
					m_pos_x *= 2;
					m_pos_y *= 4;
					mouse_motion_info = "Pressed";
					const auto selected_tool = static_cast<Tool>(rbox_tool_selected);
					const auto selected_char_mode = static_cast<CharMode>(rbox_char_mode_selected);
					const auto selected_fill_mode = static_cast<FillMode>(rbox_fill_mode_selected);

					if (selected_tool == Tool::CLIXEL) {
						switch (selected_char_mode)	{
							case CharMode::BRAILLE: 
									canvas.DrawPointLine(m_pos_x, m_pos_y, m_pos_x, m_pos_y+3, stylizer);
									canvas.DrawPointLine(m_pos_x+1, m_pos_y, m_pos_x+1, m_pos_y+3, stylizer);
								break;
							case CharMode::BLOCK: 
									canvas.DrawBlockLine(m_pos_x, m_pos_y, m_pos_x, m_pos_y+3, stylizer);
									canvas.DrawBlockLine(m_pos_x+1, m_pos_y, m_pos_x+1, m_pos_y+3, stylizer);
								break;
						}
					} else {
						if (!current_draw_action.has_value()) {
							canvas_copy = canvas;
							current_draw_action = getDrawAction(m_pos_x, m_pos_y, selected_tool, selected_char_mode, selected_fill_mode, stylizer);
						} else {
							if (prev_m_pos_x != m_pos_x || prev_m_pos_y != m_pos_y) {
								canvas = canvas_copy;
								current_draw_action->drawCallback(canvas, m_pos_x, m_pos_y);
							}
						}
					}
				} else if (mouse.motion == ftxui::Mouse::Motion::Released) {
					mouse_motion_info = "Released";
					if (current_draw_action.has_value()) {
						const auto selected_tool = static_cast<Tool>(rbox_tool_selected);
						const auto selected_char_mode = static_cast<CharMode>(rbox_char_mode_selected);
						const auto selected_fill_mode = static_cast<FillMode>(rbox_fill_mode_selected);
						figures.emplace_back(
							selected_tool, 
							selected_char_mode, 
							selected_fill_mode, 
							current_draw_action->startx(), 
							current_draw_action->starty(), 
							prev_m_pos_x, 
							prev_m_pos_y,
							r, g, b
						);
						current_draw_action.reset();
					}
				} 
				prev_m_pos_x = m_pos_x;
				prev_m_pos_y = m_pos_y;
			}
		} else if (ev == ftxui::Event::Escape) {
			if (current_draw_action.has_value()) {
				current_draw_action.reset();
				canvas = canvas_copy;
			}
		}

		return result;
	});

	scr.Loop(renderer);
}

#ifdef DEBUG
int main() {
	int argc = 7;
	const char* argv[] = {"./build-rel/src/almost-retro-paint", "--width", "128", "--height", "128", "--name", "lalal" };
#else
int main(int argc, char** argv) {
#endif
	try {
		CLI::App app{ fmt::format("{} version {}", arp::cmake::project_name, arp::cmake::project_version) };

		int width{ 0 };
		app.add_option("--width", width, "define canvas width")->required();
		int height{ 0 };
		app.add_option("--height", height, "define canvas height")->required();
		std::string file_name;
		app.add_option("--name", file_name, "define canvas file name")->required();
		bool does_load{ false };
		app.add_flag("--load", does_load, "wether to load from file in cwd of specified name with appended .json extension");

		CLI11_PARSE(app, argc, argv);

		if (does_load) {
			const auto[new_width, new_height] = load(file_name);
			width = new_width;
			height = new_height;
		}

		run(file_name, width, height);
	
	} catch (const std::exception& e) {
		spdlog::error("{}", e.what());
	}
}
