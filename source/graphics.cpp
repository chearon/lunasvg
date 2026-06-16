#include "graphics.h"
#include "lunasvg.h"

#include <cfloat>
#include <cmath>

namespace lunasvg {

const Color Color::Black(0xFF000000);
const Color Color::White(0xFFFFFFFF);
const Color Color::Transparent(0x00000000);

const Rect Rect::Empty(0, 0, 0, 0);
const Rect Rect::Invalid(0, 0, -1, -1);
const Rect Rect::Infinite(-FLT_MAX / 2.f, -FLT_MAX / 2.f, FLT_MAX, FLT_MAX);

Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

const Transform Transform::Identity(1, 0, 0, 1, 0, 0);

Transform::Transform()
{
    plutovg_matrix_init_identity(&m_matrix);
}

Transform::Transform(float a, float b, float c, float d, float e, float f)
{
    plutovg_matrix_init(&m_matrix, a, b, c, d, e, f);
}

Transform::Transform(const Matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

Transform Transform::operator*(const Transform& transform) const
{
    plutovg_matrix_t result;
    plutovg_matrix_multiply(&result, &transform.m_matrix, &m_matrix);
    return result;
}

Transform& Transform::operator*=(const Transform& transform)
{
    return (*this = *this * transform);
}

Transform& Transform::multiply(const Transform& transform)
{
    return (*this *= transform);
}

Transform& Transform::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

Transform& Transform::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

Transform& Transform::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

Transform& Transform::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

Transform& Transform::postMultiply(const Transform& transform)
{
    return (*this = transform * *this);
}

Transform& Transform::postTranslate(float tx, float ty)
{
    return postMultiply(translated(tx, ty));
}

Transform& Transform::postScale(float sx, float sy)
{
    return postMultiply(scaled(sx, sy));
}

Transform& Transform::postRotate(float angle, float cx, float cy)
{
    return postMultiply(rotated(angle, cx, cy));
}

Transform& Transform::postShear(float shx, float shy)
{
    return postMultiply(sheared(shx, shy));
}

Transform Transform::inverse() const
{
    plutovg_matrix_t inverse;
    plutovg_matrix_invert(&m_matrix, &inverse);
    return inverse;
}

Transform& Transform::invert()
{
    plutovg_matrix_invert(&m_matrix, &m_matrix);
    return *this;
}

void Transform::reset()
{
    plutovg_matrix_init_identity(&m_matrix);
}

Point Transform::mapPoint(float x, float y) const
{
    plutovg_matrix_map(&m_matrix, x, y, &x, &y);
    return Point(x, y);
}

Point Transform::mapPoint(const Point& point) const
{
    return mapPoint(point.x, point.y);
}

Rect Transform::mapRect(const Rect& rect) const
{
    if(!rect.isValid()) {
        return Rect::Invalid;
    }

    plutovg_rect_t result = {rect.x, rect.y, rect.w, rect.h};
    plutovg_matrix_map_rect(&m_matrix, &result, &result);
    return result;
}

float Transform::xScale() const
{
    return std::sqrt(m_matrix.a * m_matrix.a + m_matrix.b * m_matrix.b);
}

float Transform::yScale() const
{
    return std::sqrt(m_matrix.c * m_matrix.c + m_matrix.d * m_matrix.d);
}

bool Transform::parse(const char* data, size_t length)
{
    return plutovg_matrix_parse(&m_matrix, data, length);
}

Transform Transform::rotated(float angle, float cx, float cy)
{
    plutovg_matrix_t matrix;
    if(cx == 0.f && cy == 0.f) {
        plutovg_matrix_init_rotate(&matrix, PLUTOVG_DEG2RAD(angle));
    } else {
        plutovg_matrix_init_translate(&matrix, cx, cy);
        plutovg_matrix_rotate(&matrix, PLUTOVG_DEG2RAD(angle));
        plutovg_matrix_translate(&matrix, -cx, -cy);
    }

    return matrix;
}

Transform Transform::scaled(float sx, float sy)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_scale(&matrix, sx, sy);
    return matrix;
}

Transform Transform::sheared(float shx, float shy)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_shear(&matrix, PLUTOVG_DEG2RAD(shx), PLUTOVG_DEG2RAD(shy));
    return matrix;
}

Transform Transform::translated(float tx, float ty)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init_translate(&matrix, tx, ty);
    return matrix;
}

Path::Path(const Path& path)
    : m_data(plutovg_path_reference(path.data()))
{
}

Path::Path(Path&& path)
    : m_data(path.release())
{
}

Path::~Path()
{
    plutovg_path_destroy(m_data);
}

Path& Path::operator=(const Path& path)
{
    Path(path).swap(*this);
    return *this;
}

Path& Path::operator=(Path&& path)
{
    Path(std::move(path)).swap(*this);
    return *this;
}

void Path::moveTo(float x, float y)
{
    plutovg_path_move_to(ensure(), x, y);
}

void Path::lineTo(float x, float y)
{
    plutovg_path_line_to(ensure(), x, y);
}

void Path::quadTo(float x1, float y1, float x2, float y2)
{
    plutovg_path_quad_to(ensure(), x1, y1, x2, y2);
}

void Path::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
    plutovg_path_cubic_to(ensure(), x1, y1, x2, y2, x3, y3);
}

void Path::arcTo(float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag, float x, float y)
{
    plutovg_path_arc_to(ensure(), rx, ry, PLUTOVG_DEG2RAD(xAxisRotation), largeArcFlag, sweepFlag, x, y);
}

void Path::close()
{
    plutovg_path_close(ensure());
}

void Path::addEllipse(float cx, float cy, float rx, float ry)
{
    plutovg_path_add_ellipse(ensure(), cx, cy, rx, ry);
}

void Path::addRoundRect(float x, float y, float w, float h, float rx, float ry)
{
    plutovg_path_add_round_rect(ensure(), x, y, w, h, rx, ry);
}

void Path::addRect(float x, float y, float w, float h)
{
    plutovg_path_add_rect(ensure(), x, y, w, h);
}

void Path::addEllipse(const Point& center, const Size& radii)
{
    addEllipse(center.x, center.y, radii.w, radii.h);
}

void Path::addRoundRect(const Rect& rect, const Size& radii)
{
    addRoundRect(rect.x, rect.y, rect.w, rect.h, radii.w, radii.h);
}

void Path::addRect(const Rect& rect)
{
    addRect(rect.x, rect.y, rect.w, rect.h);
}

void Path::reset()
{
    if(m_data == nullptr)
        return;
    if(isUnique()) {
        plutovg_path_reset(m_data);
    } else {
        plutovg_path_destroy(m_data);
        m_data = nullptr;
    }
}

Rect Path::boundingRect() const
{
    if(m_data == nullptr)
        return Rect::Empty;
    plutovg_rect_t extents;
    plutovg_path_extents(m_data, &extents, false);
    return extents;
}

bool Path::isEmpty() const
{
    if(m_data)
        return plutovg_path_get_elements(m_data, nullptr) == 0;
    return true;
}

bool Path::isUnique() const
{
    return plutovg_path_get_reference_count(m_data) == 1;
}

bool Path::parse(const char* data, size_t length)
{
    plutovg_path_reset(ensure());
    return plutovg_path_parse(m_data, data, length);
}

plutovg_path_t* Path::ensure()
{
    if(isNull()) {
        m_data = plutovg_path_create();
    } else if(!isUnique()) {
        plutovg_path_destroy(m_data);
        m_data = plutovg_path_clone(m_data);
    }

    return m_data;
}

PathIterator::PathIterator(const Path& path)
    : m_size(plutovg_path_get_elements(path.data(), &m_elements))
    , m_index(0)
{
}

PathCommand PathIterator::currentSegment(std::array<Point, 3>& points) const
{
    auto command = m_elements[m_index].header.command;
    switch(command) {
    case PLUTOVG_PATH_COMMAND_MOVE_TO:
        points[0] = m_elements[m_index + 1].point;
        break;
    case PLUTOVG_PATH_COMMAND_LINE_TO:
        points[0] = m_elements[m_index + 1].point;
        break;
    case PLUTOVG_PATH_COMMAND_CUBIC_TO:
        points[0] = m_elements[m_index + 1].point;
        points[1] = m_elements[m_index + 2].point;
        points[2] = m_elements[m_index + 3].point;
        break;
    case PLUTOVG_PATH_COMMAND_CLOSE:
        points[0] = m_elements[m_index + 1].point;
        break;
    }

    return PathCommand(command);
}

void PathIterator::next()
{
    m_index += m_elements[m_index].header.length;
}

FontFace::FontFace(cairo_font_face_t* face)
    : m_face(cairo_font_face_reference(face))
{
}

FontFace::FontFace(const FontFace& face)
    : m_face(cairo_font_face_reference(face.get()))
{
}

FontFace::FontFace(FontFace&& face)
    : m_face(face.release())
{
}

FontFace::~FontFace()
{
    cairo_font_face_destroy(m_face);
}

FontFace& FontFace::operator=(const FontFace& face)
{
    FontFace(face).swap(*this);
    return *this;
}

FontFace& FontFace::operator=(FontFace&& face)
{
    FontFace(std::move(face)).swap(*this);
    return *this;
}

void FontFace::swap(FontFace& face)
{
    std::swap(m_face, face.m_face);
}

cairo_font_face_t* FontFace::release()
{
    return std::exchange(m_face, nullptr);
}

FontFace GraphicsCallbacks::getFontFace(const std::string& family, bool bold, bool italic) const
{
    if (this->retriever) return this->retriever(family, bold, italic);
    return FontFace();
}

cairo_surface_t* GraphicsCallbacks::createSurfaceForImage(char* data, int length) {
    if (this->decoder) return this->decoder(data, length);
    return nullptr;
}

void GraphicsCallbacks::setRetrieverFn(FontFace (*fn)(const std::string&, bool, bool))
{
    this->retriever = fn;
}

void GraphicsCallbacks::setDecoderFn(cairo_surface_t* (*fn)(char*, int))
{
    this->decoder = fn;
}

Font::Font(const FontFace& face, float size)
    : m_face(face), m_size(size)
{
    if(m_size > 0.f && !m_face.isNull()) {
        m_ascent = face.m_ascent / 1000.0 * m_size;
        m_descent = face.m_descent / 1000.0 * m_size;
        m_lineGap = face.m_lineGap / 1000.0 * m_size;
        m_xHeight = face.m_xHeight / 1000.0 * m_size;
    }
}

float Font::xHeight() const
{
    return m_xHeight;
}

float Font::measureText(const std::u32string_view& text) const
{
    if(m_size > 0.f && !m_face.isNull() && this->measurer)
        return (*this->measurer)(text, *this);
    return 0;
}

void Font::paintText(const std::u32string_view text, const Point& point, const Transform& transform, float strokeWidth) const
{
    if (this->painter) {
        (*this->painter)(text, *this, point, transform, strokeWidth);
    }
}

std::shared_ptr<Canvas> Canvas::create(const Bitmap& bitmap)
{
    return std::shared_ptr<Canvas>(new Canvas(bitmap));
}

std::shared_ptr<Canvas> Canvas::create(float x, float y, float width, float height)
{
    constexpr int kMaxSize = 1 << 15;
    if(width <= 0 || height <= 0 || width >= kMaxSize || height >= kMaxSize)
        return std::shared_ptr<Canvas>(new Canvas(0, 0, 1, 1));
    auto l = static_cast<int>(std::floor(x));
    auto t = static_cast<int>(std::floor(y));
    auto r = static_cast<int>(std::ceil(x + width));
    auto b = static_cast<int>(std::ceil(y + height));
    return std::shared_ptr<Canvas>(new Canvas(l, t, r - l, b - t));
}

std::shared_ptr<Canvas> Canvas::create(const Rect& extents)
{
    return create(extents.x, extents.y, extents.w, extents.h);
}

void Canvas::setColor(const Color& color)
{
    setColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

void Canvas::setColor(float r, float g, float b, float a)
{
    cairo_set_source_rgba(m_canvas, r, g, b, a);
}

static cairo_extend_t convert_spread(SpreadMethod spread)
{
    switch (spread) {
        case SpreadMethod::Pad: return CAIRO_EXTEND_PAD;
        case SpreadMethod::Reflect: return CAIRO_EXTEND_REFLECT;
        case SpreadMethod::Repeat: return CAIRO_EXTEND_REPEAT;
    }
}

static void convert_matrix(cairo_matrix_t* dst, const plutovg_matrix_t* src)
{
    dst->xx = src->a;
    dst->yx = src->b;
    dst->xy = src->c;
    dst->yy = src->d;
    dst->x0 = src->e;
    dst->y0 = src->f;
}

static void add_pattern_matrix(cairo_pattern_t* pattern, const plutovg_matrix_t* matrix) {
  cairo_matrix_t m;
  convert_matrix(&m, matrix);
  cairo_matrix_invert(&m);
  cairo_pattern_set_matrix(pattern, &m);
}

void Canvas::setLinearGradient(float x1, float y1, float x2, float y2, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    cairo_pattern_t* gradient = cairo_pattern_create_linear(x1, y1, x2, y2);
    for (auto& stop : stops) {
      cairo_pattern_add_color_stop_rgba(
          gradient,
          stop.offset,
          stop.color.r,
          stop.color.g,
          stop.color.b,
          stop.color.a
      );
    }
    cairo_pattern_set_extend(gradient, convert_spread(spread));
    add_pattern_matrix(gradient, &transform.matrix());
    cairo_set_source(m_canvas, gradient);
    cairo_pattern_destroy(gradient);
}

static cairo_extend_t convert_texture_type(TextureType type) {
    switch (type) {
        case TextureType::Plain: return CAIRO_EXTEND_NONE;
        case TextureType::Tiled: return CAIRO_EXTEND_REPEAT;
    }
}

void Canvas::setRadialGradient(float cx, float cy, float r, float fx, float fy, SpreadMethod spread, const GradientStops& stops, const Transform& transform)
{
    cairo_pattern_t* gradient = cairo_pattern_create_radial(cx, cy, 0, fx, fy, r);
    for (auto& stop : stops) {
      cairo_pattern_add_color_stop_rgba(
          gradient,
          stop.offset,
          stop.color.r,
          stop.color.g,
          stop.color.b,
          stop.color.a
      );
    }
    cairo_pattern_set_extend(gradient, convert_spread(spread));
    add_pattern_matrix(gradient, &transform.matrix());
    cairo_set_source(m_canvas, gradient);
    cairo_pattern_destroy(gradient);
}

void Canvas::setTexture(const Canvas& source, TextureType type, float opacity, const Transform& transform)
{
    cairo_pattern_t* texture = cairo_pattern_create_for_surface(source.surface());
    cairo_pattern_set_extend(texture, convert_texture_type(type));
    add_pattern_matrix(texture, &transform.matrix());
    cairo_set_source(m_canvas, texture);
    cairo_pattern_destroy(texture);
}

static cairo_fill_rule_t convert_fill_rule(FillRule rule) {
    switch (rule) {
        case FillRule::NonZero: return CAIRO_FILL_RULE_WINDING;
        case FillRule::EvenOdd: return CAIRO_FILL_RULE_EVEN_ODD;
    }
}

static void add_path(cairo_t* cr, const Path& path) {
    PathIterator it(path);
    std::array<Point, 3> p;

    while(!it.isDone()) {
        switch(it.currentSegment(p)) {
            case PathCommand::MoveTo:
                cairo_move_to(cr, p[0].x, p[0].y);
                break;
            case PathCommand::LineTo:
                cairo_line_to(cr, p[0].x, p[0].y);
                break;
            case PathCommand::CubicTo:
                cairo_curve_to(cr, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y);
                break;
            case PathCommand::Close:
                cairo_close_path(cr);
                break;
        }

        it.next();
    }
}

static void add_matrix(cairo_t* cr, const plutovg_matrix_t* p_m1, const plutovg_matrix_t* p_m2) {
    cairo_matrix_t m1;
    convert_matrix(&m1, p_m1);
    cairo_set_matrix(cr, &m1);
    if (p_m2 != nullptr) { 
      cairo_matrix_t m2;
      convert_matrix(&m2, p_m2);
      cairo_transform(cr, &m2);
    }
}

void Canvas::fillPath(const Path& path, FillRule fillRule, const Transform& transform)
{
    add_matrix(m_canvas, &m_translation, &transform.matrix());
    cairo_set_fill_rule(m_canvas, convert_fill_rule(fillRule));
    add_path(m_canvas, path);
    cairo_fill(m_canvas);
}

static cairo_line_cap_t convert_line_cap(LineCap cap) {
    switch (cap) {
        case LineCap::Butt: return CAIRO_LINE_CAP_BUTT;
        case LineCap::Round: return CAIRO_LINE_CAP_ROUND;
        case LineCap::Square: return CAIRO_LINE_CAP_SQUARE;
    }
}

static cairo_line_join_t convert_line_join(LineJoin join) {
    switch (join) {
        case LineJoin::Miter: return CAIRO_LINE_JOIN_MITER;
        case LineJoin::Round: return CAIRO_LINE_JOIN_ROUND;
        case LineJoin::Bevel: return CAIRO_LINE_JOIN_BEVEL;
    }
}

void Canvas::strokePath(const Path& path, const StrokeData& strokeData, const Transform& transform)
{
    double dashes[256];
    size_t dash_idx = 0;

    add_matrix(m_canvas, &m_translation, &transform.matrix());
    cairo_set_line_width(m_canvas, strokeData.lineWidth());
    cairo_set_miter_limit(m_canvas, strokeData.miterLimit());
    cairo_set_line_cap(m_canvas, convert_line_cap(strokeData.lineCap()));
    cairo_set_line_join(m_canvas, convert_line_join(strokeData.lineJoin()));

    for (float dash : strokeData.dashArray()) {
      if (dash_idx < (sizeof(dashes) / sizeof(dashes[0]))) {
        dashes[dash_idx++] = (double)dash;
      } else break;
    }

    cairo_set_dash(m_canvas, dashes, dash_idx, strokeData.dashOffset());
    add_path(m_canvas, path);
    cairo_stroke(m_canvas);
}

void Canvas::fillText(const std::u32string_view& text, const Font& font, const Point& origin, const Transform& transform)
{
  font.paintText(text, origin, transform, -1);
}

void Canvas::strokeText(const std::u32string_view& text, float strokeWidth, const Font& font, const Point& origin, const Transform& transform)
{
  font.paintText(text, origin, transform, strokeWidth);
}

void Canvas::clipPath(const Path& path, FillRule clipRule, const Transform& transform)
{
    add_matrix(m_canvas, &m_translation, &transform.matrix());
    cairo_set_fill_rule(m_canvas, convert_fill_rule(clipRule));
    add_path(m_canvas, path);
    cairo_clip(m_canvas);
}

void Canvas::clipRect(const Rect& rect, FillRule clipRule, const Transform& transform)
{
    add_matrix(m_canvas, &m_translation, &transform.matrix());
    cairo_set_fill_rule(m_canvas, convert_fill_rule(clipRule));
    cairo_rectangle(m_canvas, rect.x, rect.y, rect.w, rect.h);
    cairo_clip(m_canvas);
}

void Canvas::drawImage(const Bitmap& image, const Rect& dstRect, const Rect& srcRect, const Transform& transform)
{
    auto xScale = dstRect.w / srcRect.w;
    auto yScale = dstRect.h / srcRect.h;
    plutovg_matrix_t matrix = { xScale, 0, 0, yScale, -srcRect.x * xScale, -srcRect.y * yScale };
    add_matrix(m_canvas, &m_translation, &transform.matrix());
    cairo_translate(m_canvas, dstRect.x, dstRect.y);
    cairo_set_fill_rule(m_canvas, CAIRO_FILL_RULE_WINDING);
    cairo_pattern_t* texture = cairo_pattern_create_for_surface(image.surface());
    cairo_pattern_set_extend(texture, CAIRO_EXTEND_REPEAT);
    add_pattern_matrix(texture, &matrix);
    cairo_set_source(m_canvas, texture);
    cairo_pattern_destroy(texture);
    cairo_rectangle(m_canvas, 0, 0, dstRect.w, dstRect.h);
    cairo_fill(m_canvas);
}

static cairo_operator_t convert_blend(BlendMode mode) {
    switch (mode) {
        case BlendMode::Src: return CAIRO_OPERATOR_SOURCE;
        case BlendMode::Src_Over: return CAIRO_OPERATOR_OVER;
        case BlendMode::Dst_In: return CAIRO_OPERATOR_DEST_IN;
        case BlendMode::Dst_Out: return CAIRO_OPERATOR_DEST_OUT;
    }
}

void Canvas::blendCanvas(const Canvas& canvas, BlendMode blendMode, float opacity)
{
    plutovg_matrix_t matrix = { 1, 0, 0, 1, static_cast<float>(canvas.x()), static_cast<float>(canvas.y()) };
    add_matrix(m_canvas, &m_translation, nullptr);
    cairo_set_operator(m_canvas, convert_blend(blendMode));
    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(canvas.surface());
    add_pattern_matrix(pattern, &matrix);
    cairo_set_source(m_canvas, pattern);
    cairo_pattern_destroy(pattern);
    cairo_paint_with_alpha(m_canvas, opacity);
}

void Canvas::save()
{
    cairo_save(m_canvas);
}

void Canvas::restore()
{
    cairo_restore(m_canvas);
}

int Canvas::width() const
{
    return cairo_image_surface_get_width(m_surface);
}

int Canvas::height() const
{
    return cairo_image_surface_get_height(m_surface);
}

void Canvas::convertToLuminanceMask()
{
    auto width = cairo_image_surface_get_width(m_surface);
    auto height = cairo_image_surface_get_height(m_surface);
    auto stride = cairo_image_surface_get_stride(m_surface);
    auto data = cairo_image_surface_get_data(m_surface);
    for(int y = 0; y < height; y++) {
        auto pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; x++) {
            auto pixel = pixels[x];
            auto a = (pixel >> 24) & 0xFF;
            auto r = (pixel >> 16) & 0xFF;
            auto g = (pixel >> 8) & 0xFF;
            auto b = (pixel >> 0) & 0xFF;
            if(a) {
                r = (r * 255) / a;
                g = (g * 255) / a;
                b = (b * 255) / a;
            }

            auto l = (r * 0.2125 + g * 0.7154 + b * 0.0721);
            pixels[x] = static_cast<uint32_t>(l * (a / 255.0)) << 24;
        }
    }
}

Canvas::~Canvas()
{
    cairo_destroy(m_canvas);
    cairo_surface_destroy(m_surface);
}

Canvas::Canvas(const Bitmap& bitmap)
    : m_surface(cairo_surface_reference(bitmap.surface()))
    , m_canvas(cairo_create(m_surface))
    , m_translation({1, 0, 0, 1, 0, 0})
    , m_x(0), m_y(0)
{
}

Canvas::Canvas(int x, int y, int width, int height)
    : m_surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height))
    , m_canvas(cairo_create(m_surface))
    , m_translation({1, 0, 0, 1, -static_cast<float>(x), -static_cast<float>(y)})
    , m_x(x), m_y(y)
{
}

} // namespace lunasvg
