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

#include <config.hpp>

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
	enum class CharMode { BRAILLE, BLOCK };
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
	enum class Tool { CLIXEL, LINE, CIRCLE, ELLIPSE };
	const std::vector<std::string> tools{
		"clixel", // 0
		"line",   // 1
		"circle", // 2
		"elipse", // 3
	};
	int rbox_tool_selected{ 0 };
	auto rbox_tools = ftxui::Radiobox(&tools, &rbox_tool_selected);
	/// fill mode
	enum class FillMode { FILLED, EMPTY };
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
	});
	/// save canvas button
	//const std::string button_save_canvas_text = "      Save";
	//auto button_save_canvas = ftxui::Button(button_save_canvas_text, [&]{
	//	save(file_name, working_dir, canvas);
	//});
	
	/// container
	auto lhs_panel_container = ftxui::Container::Vertical({
		rbox_char_modes,
		cboxes_style_container,
		rbox_fill_modes,
		rbox_tools,
		button_clear_canvas,
		//button_save_canvas,
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
	auto sliders_container = ftxui::Container::Vertical({
		slider_r, slider_g, slider_b	
	});
	auto rhs_panel_container = ftxui::Container::Vertical({
		button_close,
		sliders_container
	});
	/*
			Organize app panel into container
	*/
	auto app_ui_container = ftxui::Container::Horizontal({
		lhs_panel_container,
		canvas_renderer,
		rhs_panel_container
	});

	const auto draw = [&] {
		return ftxui::hbox({
			ftxui::window(ftxui::text(""), ftxui::vbox({
				ftxui::window(ftxui::text("char"), rbox_char_modes->Render()),
				ftxui::window(ftxui::text("styles"), cboxes_style_container->Render()),
				ftxui::window(ftxui::text("fill mode"), rbox_fill_modes->Render()),
				ftxui::window(ftxui::text("tool"), rbox_tools->Render()),
				button_clear_canvas->Render(), 
				ftxui::separator(),
				//button_save_canvas->Render(),
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
				}))
			}))
		});
	};

	int prev_m_pos_x{ 0 };
	int prev_m_pos_y{ 0 };
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
							switch (selected_tool) {
								case Tool::LINE:    
										current_draw_action.emplace(
											selected_char_mode == CharMode::BRAILLE ?
											DrawAction(m_pos_x, m_pos_y, 
											[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
												canvas.DrawPointLine(start_x, start_y, end_x, end_y, stylizer);
											}) :
											DrawAction(m_pos_x, m_pos_y, 
											[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
												canvas.DrawBlockLine(start_x, start_y, end_x, end_y, stylizer);
											})   
										);
									break;
								case Tool::CIRCLE:  
										if (selected_char_mode == CharMode::BRAILLE) {
											current_draw_action.emplace(
												selected_fill_mode == FillMode::FILLED ?
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													const int x_len = end_x - start_x;
													const int y_len = end_y - start_y;
													const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
													canvas.DrawPointCircleFilled(start_x, start_y, len, stylizer);
												}) :
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													const int x_len = end_x - start_x;
													const int y_len = end_y - start_y;
													const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
													canvas.DrawPointCircle(start_x, start_y, len, stylizer);
												})   
											);
										} else {
											current_draw_action.emplace(
												selected_fill_mode == FillMode::FILLED ?
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													const int x_len = end_x - start_x;
													const int y_len = end_y - start_y;
													const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
													canvas.DrawBlockCircleFilled(start_x, start_y, len, stylizer);
												}) :
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													const int x_len = end_x - start_x;
													const int y_len = end_y - start_y;
													const int len = static_cast<int>(std::sqrt(x_len*x_len + y_len*y_len));
													canvas.DrawBlockCircle(start_x, start_y, len, stylizer);
												})   
											);
										}
									break;
								case Tool::ELLIPSE:
										if (selected_char_mode == CharMode::BRAILLE) {
											current_draw_action.emplace(
												selected_fill_mode == FillMode::FILLED ?
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													canvas.DrawPointEllipseFilled(start_x, start_y, end_x, end_y, stylizer);
												}) :
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													canvas.DrawPointEllipse(start_x, start_y, end_x, end_y, stylizer);
												})   
											);
										} else {
											current_draw_action.emplace(
												selected_fill_mode == FillMode::FILLED ?
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													canvas.DrawBlockEllipseFilled(start_x, start_y, end_x, end_y, stylizer);
												}) :
												DrawAction(m_pos_x, m_pos_y, 
												[selected_char_mode, &stylizer](ftxui::Canvas& canvas, int start_x, int start_y, int end_x, int end_y) {
													canvas.DrawBlockEllipse(start_x, start_y, end_x, end_y, stylizer);
												})   
											);
										}
								break;
								default: break;
							}
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

int main(int argc, const char** argv) {
	try {
		CLI::App app{ fmt::format("{} version {}", arp::cmake::project_name, arp::cmake::project_version) };

		int width{ 0 };
		app.add_option("--width", width, "define canvas width")->required();
		int height{ 0 };
		app.add_option("--height", height, "define canvas height")->required();
		std::string file_name;
		app.add_option("--name", file_name, "define canvas file name")->required();

		CLI11_PARSE(app, argc, argv);

		run(file_name, width, height);
	
	} catch (const std::exception& e) {
		spdlog::error("{}", e.what());
	}
}
