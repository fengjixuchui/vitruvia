/*
	____   ____.__  __                     .__        
	\   \ /   /|__|/  |________ __ _____  _|__|____   
	 \   Y   / |  \   __\_  __ \  |  \  \/ /  \__  \  
	  \     /  |  ||  |  |  | \/  |  /\   /|  |/ __ \_
	   \___/   |__||__|  |__|  |____/  \_/ |__(____  /
												   \/ 
	Premium Internal Multihack for Counter-Strike: Global Offensive
	Made with love by double v - 2017
*/

#include "D3D_Renderer.h"

namespace Vitruvia {
namespace Render {

namespace D3D
{
	DrawManager* g_pRenderer;

	bool get_system_font_path(const std::string& name, std::string& path)
	{
		//
		// This code is not as safe as it should be.
		// Assumptions we make:
		//  -> GetWindowsDirectoryA does not fail.
		//  -> The registry key exists.
		//  -> The subkeys are ordered alphabetically
		//  -> The subkeys name and data are no longer than 260 (MAX_PATH) chars.
		//

		char buffer[MAX_PATH];
		HKEY registryKey;

		GetWindowsDirectoryA(buffer, MAX_PATH);
		std::string fontsFolder = buffer + std::string("\\Fonts\\");

		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &registryKey)) {
			return false;
		}

		uint32_t valueIndex = 0;
		char valueName[MAX_PATH];
		uint8_t valueData[MAX_PATH];
		std::wstring wsFontFile;

		do {
			uint32_t valueNameSize = MAX_PATH;
			uint32_t valueDataSize = MAX_PATH;
			uint32_t valueType;

			auto error = RegEnumValueA(
				registryKey,
				valueIndex,
				valueName,
				reinterpret_cast<DWORD*>(&valueNameSize),
				0,
				reinterpret_cast<DWORD*>(&valueType),
				valueData,
				reinterpret_cast<DWORD*>(&valueDataSize));

			valueIndex++;

			if (error == ERROR_NO_MORE_ITEMS) {
				RegCloseKey(registryKey);
				return false;
			}

			if (error || valueType != REG_SZ) {
				continue;
			}

			if (_strnicmp(name.data(), valueName, name.size()) == 0) {
				path = fontsFolder + std::string((char*)valueData, valueDataSize);
				RegCloseKey(registryKey);
				return true;
			}
		} while (true);

		return false;
	}

	DrawManager::DrawManager(IDirect3DDevice9* device)
	{
		_device = device;
		_texture = nullptr;
		_drawList = nullptr;
	}

	DrawManager::~DrawManager()
	{
		this->InvalidateObjects();
	}

	void DrawManager::CreateObjects()
	{
		_drawList = new ImDrawList();

		auto font_path = std::string{};

		uint8_t* pixel_data;

		int width,
			height,
			bytes_per_pixel;

		if (!get_system_font_path(FontName, font_path))
		{
			// If it returned false, avoid returning prematurely.
			// It MAY mean that it didn't find the font, so re-do it again with Tahoma
			if (!get_system_font_path(enc("Verdana"), font_path))
				return;
		}

		auto font = _fonts.AddFontFromFileTTF(font_path.data(), FontSize, 0, _fonts.GetGlyphRangesDefault());

		_fonts.GetTexDataAsRGBA32(&pixel_data, &width, &height, &bytes_per_pixel);

		auto hr = _device->CreateTexture(
			width, height,
			1,
			D3DUSAGE_DYNAMIC,
			D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT,
			&_texture,
			NULL);

		if (FAILED(hr)) return;

		D3DLOCKED_RECT tex_locked_rect;
		if (_texture->LockRect(0, &tex_locked_rect, NULL, 0) != D3D_OK)
			return;
		for (int y = 0; y < height; y++)
			memcpy((uint8_t*)tex_locked_rect.pBits + tex_locked_rect.Pitch * y, pixel_data + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
		_texture->UnlockRect(0);

		_fonts.TexID = _texture;
	}

	void DrawManager::InvalidateObjects()
	{
		if (_texture) _texture->Release();
		_texture = nullptr;

		_fonts.Clear();

		if (_drawList)
			delete _drawList;
		_drawList = nullptr;
	}

	void DrawManager::BeginRendering()
	{
		_drawData.Valid = false;

		_drawList->Clear();
		_drawList->PushClipRectFullScreen();
	}

	void DrawManager::EndRendering()
	{
		ImGui_ImplDX9_RenderDrawLists(GetDrawData());
	}

	void DrawManager::Text(ImVec2 point, ImU32 color, text_flags flags, const char* format, ...)
	{
		static const auto MAX_BUFFER_SIZE = 1024;
		static char buffer[MAX_BUFFER_SIZE] = "";

		auto font = _fonts.Fonts[0];

		_drawList->PushTextureID(_fonts.TexID);

		va_list va;
		va_start(va, format);
		vsnprintf_s(buffer, MAX_BUFFER_SIZE, format, va);
		va_end(va);

		if (flags & centered_x || flags & centered_y) {
			auto text_size = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, buffer);
			if (flags & centered_x)
				point.x -= text_size.x / 2;
			if (flags & centered_y)
				point.y -= text_size.y / 2;
		}

		if (flags & outline) {
			_drawList->AddText(font, font->FontSize, ImVec2{ point.x - 1, point.y - 1 }, 0xFF000000, buffer);
			_drawList->AddText(font, font->FontSize, ImVec2{ point.x + 1, point.y }, 0xFF000000, buffer);
			_drawList->AddText(font, font->FontSize, ImVec2{ point.x    , point.y + 1 }, 0xFF000000, buffer);
			_drawList->AddText(font, font->FontSize, ImVec2{ point.x - 1, point.y }, 0xFF000000, buffer);
		}

		if (flags & drop_shadow && !(flags & outline)) {
			_drawList->AddText(font, font->FontSize, ImVec2{ point.x + 1, point.y + 1 }, 0xFF000000, buffer);
		}

		_drawList->AddText(font, font->FontSize, point, color, buffer);
		_drawList->PopTextureID();
	}

	void DrawManager::Rect(const ImVec2& a, float w, float h, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/, float thickness /*= 1.0f*/)
	{
		_drawList->AddRect(a, { a.x + w, a.y + h }, col, rounding, rounding_corners_flags, thickness);
	}

	void DrawManager::Line(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness /*= 1.0f*/)
	{
		_drawList->AddLine(a, b, col, thickness);
	}

	void DrawManager::Line(const ImVec2& a, float w, float h, ImU32 col, float thickness /*= 1.0f*/)
	{
		_drawList->AddLine(a, { a.x + w, a.y + h }, col, thickness);
	}

	void DrawManager::Rect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/, float thickness /*= 1.0f*/)
	{
		_drawList->AddRect(a, b, col, rounding, rounding_corners_flags, thickness);
	}

	void DrawManager::RectFilled(const ImVec2& a, float w, float h, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/)
	{
		_drawList->AddRectFilled(a, { a.x + w, a.y + h }, col, rounding, rounding_corners_flags);
	}

	void DrawManager::RectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding /*= 0.0f*/, int rounding_corners_flags /*= ~0*/)
	{
		_drawList->AddRectFilled(a, b, col, rounding, rounding_corners_flags);
	}

	void DrawManager::RectFilledMultiColor(const ImVec2& a, const ImVec2& b, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
	{
		_drawList->AddRectFilledMultiColor(a, b, col_upr_left, col_upr_right, col_bot_right, col_bot_left);
	}

	void DrawManager::Quad(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col, float thickness /*= 1.0f*/)
	{
		_drawList->AddQuad(a, b, c, d, col, thickness);
	}

	void DrawManager::QuadFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col)
	{
		_drawList->AddQuadFilled(a, b, c, d, col);
	}

	void DrawManager::Triangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col, float thickness /*= 1.0f*/)
	{
		_drawList->AddTriangle(a, b, c, col, thickness);
	}

	void DrawManager::TriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col)
	{
		_drawList->AddTriangleFilled(a, b, c, col);
	}

	void DrawManager::Circle(const ImVec2& centre, float radius, ImU32 col, int num_segments /*= 12*/, float thickness /*= 1.0f*/)
	{
		_drawList->AddCircle(centre, radius, col, num_segments, thickness);
	}

	void DrawManager::CircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments /*= 12*/)
	{
		_drawList->AddCircleFilled(centre, radius, col, num_segments);
	}

	void DrawManager::Polyline(const ImVec2* points, const int num_points, ImU32 col, bool closed, float thickness, bool anti_aliased)
	{
		_drawList->AddPolyline(points, num_points, col, closed, thickness, anti_aliased);
	}

	void DrawManager::ConvexPolyFilled(const ImVec2* points, const int num_points, ImU32 col, bool anti_aliased)
	{
		_drawList->AddConvexPolyFilled(points, num_points, col, anti_aliased);
	}

	void DrawManager::BezierCurve(const ImVec2& pos0, const ImVec2& cp0, const ImVec2& cp1, const ImVec2& pos1, ImU32 col, float thickness, int num_segments /*= 0*/)
	{
		_drawList->AddBezierCurve(pos0, cp0, cp1, pos1, col, thickness, num_segments);
	}

	bool DrawManager::W2S(const Source::Vector& origin, Source::Vector& screen)
	{
		GRAB_SCREEN_SIZE

		static auto ViewMatrix = rcast<uintptr_t>(&Source::Engine->WorldToScreenMatrix()) + 0x40;
		if (ViewMatrix != NULL)
		{
			auto result = *rcast<uintptr_t*>(ViewMatrix) + 0x3DC;
			if (result != NULL)
			{
				const auto& WorldToScreenMatrix = *rcast<D3DMATRIX*>(result);
				if (WorldToScreenMatrix.m != NULL)
				{
					auto width = WorldToScreenMatrix._41 * origin.x + WorldToScreenMatrix._42 * origin.y + WorldToScreenMatrix._43 * origin.z + WorldToScreenMatrix._44;

					if (width < 0.001)
						return false;

					auto cx = static_cast<float>(screen_width) / 2.0f;
					auto cy = static_cast<float>(screen_height) / 2.0f;

					screen.x = WorldToScreenMatrix._11 * origin.x + WorldToScreenMatrix._12 * origin.y + WorldToScreenMatrix._13 * origin.z + WorldToScreenMatrix._14;
					screen.y = WorldToScreenMatrix._21 * origin.x + WorldToScreenMatrix._22 * origin.y + WorldToScreenMatrix._23 * origin.z + WorldToScreenMatrix._24;

					screen.x /= width;
					screen.y /= width;

					screen.x = cx + cx * screen.x;
					screen.y = cy - cy * screen.y;
					screen.z = 0;

					return true;
				}
			}
		}
		return false;
	}

	ImDrawData* DrawManager::GetDrawData()
	{
		if (!_drawList->VtxBuffer.empty()) {
			_drawData.Valid = true;
			_drawData.CmdLists = &_drawList;
			_drawData.CmdListsCount = 1;
			_drawData.TotalVtxCount = _drawList->VtxBuffer.Size;
			_drawData.TotalIdxCount = _drawList->IdxBuffer.Size;
		}
		return &_drawData;
	}
}

}
}
