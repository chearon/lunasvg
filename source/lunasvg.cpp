#include "lunasvg.h"
#include "svgelement.h"
#include "svgrenderstate.h"

#include <cstring>
#include <fstream>
#include <cmath>

int lunasvg_version()
{
    return LUNASVG_VERSION;
}

const char* lunasvg_version_string()
{
    return LUNASVG_VERSION_STRING;
}

namespace lunasvg {

Bitmap::Bitmap(int width, int height)
    : m_surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height))
{
}

Bitmap::Bitmap(const Bitmap& bitmap)
    : m_surface(cairo_surface_reference(bitmap.surface()))
{
}

Bitmap::Bitmap(Bitmap&& bitmap)
    : m_surface(bitmap.release())
{
}

Bitmap::~Bitmap()
{
    cairo_surface_destroy(m_surface);
}

Bitmap& Bitmap::operator=(const Bitmap& bitmap)
{
    Bitmap(bitmap).swap(*this);
    return *this;
}

void Bitmap::swap(Bitmap& bitmap)
{
    std::swap(m_surface, bitmap.m_surface);
}

uint8_t* Bitmap::data() const
{
    if(m_surface)
        return cairo_image_surface_get_data(m_surface);
    return nullptr;
}

int Bitmap::width() const
{
    if(m_surface)
        return cairo_image_surface_get_width(m_surface);
    return 0;
}

int Bitmap::height() const
{
    if(m_surface)
        return cairo_image_surface_get_height(m_surface);
    return 0;
}

int Bitmap::stride() const
{
    if(m_surface)
        return cairo_image_surface_get_stride(m_surface);
    return 0;
}

void Bitmap::clear(uint32_t value)
{
    if(m_surface == nullptr)
        return;
    plutovg_color_t color;
    plutovg_color_init_rgba32(&color, value);
    cairo_t* cr = cairo_create(m_surface);
    cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
    cairo_rectangle(cr, 0, 0, width(), height());
    cairo_fill(cr);
    cairo_destroy(cr);
}

Bitmap& Bitmap::operator=(Bitmap&& bitmap)
{
    Bitmap(std::move(bitmap)).swap(*this);
    return *this;
}

bool Bitmap::writeToPng(const std::string& filename) const
{
    if(m_surface)
        return cairo_surface_write_to_png(m_surface, filename.data());
    return false;
}

cairo_surface_t* Bitmap::release()
{
    return std::exchange(m_surface, nullptr);
}

Box::Box(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

Box::Box(const Rect& rect)
    : x(rect.x), y(rect.y), w(rect.w), h(rect.h)
{
}

Box& Box::transform(const Matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

Box Box::transformed(const Matrix& matrix) const
{
    return Transform(matrix).mapRect(*this);
}

Matrix::Matrix(float a, float b, float c, float d, float e, float f)
    : a(a), b(b), c(c), d(d), e(e), f(f)
{
}

Matrix::Matrix(const plutovg_matrix_t& matrix)
    : a(matrix.a), b(matrix.b), c(matrix.c), d(matrix.d), e(matrix.e), f(matrix.f)
{
}

Matrix::Matrix(const Transform& transform)
    : Matrix(transform.matrix())
{
}

Matrix Matrix::operator*(const Matrix& matrix) const
{
    return Transform(*this) * Transform(matrix);
}

Matrix& Matrix::operator*=(const Matrix &matrix)
{
    return (*this = *this * matrix);
}

Matrix& Matrix::multiply(const Matrix& matrix)
{
    return (*this *= matrix);
}

Matrix& Matrix::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

Matrix& Matrix::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

Matrix& Matrix::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

Matrix& Matrix::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

Matrix Matrix::inverse() const
{
    return Transform(*this).inverse();
}

Matrix& Matrix::invert()
{
    return (*this = inverse());
}

void Matrix::reset()
{
    *this = Matrix(1, 0, 0, 1, 0, 0);
}

Matrix Matrix::translated(float tx, float ty)
{
    return Transform::translated(tx, ty);
}

Matrix Matrix::scaled(float sx, float sy)
{
    return Transform::scaled(sx, sy);
}

Matrix Matrix::rotated(float angle, float cx, float cy)
{
    return Transform::rotated(angle, cx, cy);
}

Matrix Matrix::sheared(float shx, float shy)
{
    return Transform::sheared(shx, shy);
}

Node::Node(SVGNode* node)
    : m_node(node)
{
}

bool Node::isTextNode() const
{
    return m_node && m_node->isTextNode();
}

bool Node::isElement() const
{
    return m_node && m_node->isElement();
}

TextNode Node::toTextNode() const
{
    if(m_node && m_node->isTextNode())
        return static_cast<SVGTextNode*>(m_node);
    return TextNode();
}

Element Node::toElement() const
{
    if(m_node && m_node->isElement())
        return static_cast<SVGElement*>(m_node);
    return Element();
}

Element Node::parentElement() const
{
    if(m_node)
        return m_node->parentElement();
    return Element();
}

TextNode::TextNode(SVGTextNode* text)
    : Node(text)
{
}

const std::string& TextNode::data() const
{
    if(m_node)
        return text()->data();
    return emptyString;
}

void TextNode::setData(const std::string& data)
{
    if(m_node) {
        text()->setData(data);
    }
}

SVGTextNode* TextNode::text() const
{
    return static_cast<SVGTextNode*>(m_node);
}

Element::Element(SVGElement* element)
    : Node(element)
{
}

bool Element::hasAttribute(const std::string& name) const
{
    if(m_node)
        return element()->hasAttribute(name);
    return false;
}

const std::string& Element::getAttribute(const std::string& name) const
{
    if(m_node)
        return element()->getAttribute(name);
    return emptyString;
}

void Element::setAttribute(const std::string& name, const std::string& value)
{
    if(m_node) {
        element()->setAttribute(name, value);
    }
}

void Element::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(m_node == nullptr || bitmap.isNull())
        return;
    auto canvas = Canvas::create(bitmap);
    SVGRenderState state(nullptr, nullptr, matrix, SVGRenderMode::Painting, canvas);
    element(true)->render(state);
}

Bitmap Element::renderToBitmap(int width, int height, uint32_t backgroundColor) const
{
    if(m_node == nullptr)
        return Bitmap();
    auto elementBounds = element(true)->localTransform().mapRect(element()->paintBoundingBox());
    if(elementBounds.isEmpty())
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(elementBounds.w));
        height = static_cast<int>(std::ceil(elementBounds.h));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * elementBounds.h / elementBounds.w));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * elementBounds.w / elementBounds.h));
    }

    auto xScale = width / elementBounds.w;
    auto yScale = height / elementBounds.h;

    Matrix matrix(xScale, 0, 0, yScale, -elementBounds.x * xScale, -elementBounds.y * yScale);
    Bitmap bitmap(width, height);
    if(backgroundColor) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

Matrix Element::getLocalMatrix() const
{
    if(m_node)
        return element(true)->localTransform();
    return Matrix();
}

Matrix Element::getGlobalMatrix() const
{
    if(m_node == nullptr)
        return Matrix();
    auto transform = element(true)->localTransform();
    for(auto parent = element()->parentElement(); parent; parent = parent->parentElement())
        transform.postMultiply(parent->localTransform());
    return transform;
}

Box Element::getLocalBoundingBox() const
{
    return getBoundingBox().transformed(getLocalMatrix());
}

Box Element::getGlobalBoundingBox() const
{
    return getBoundingBox().transformed(getGlobalMatrix());
}

Box Element::getBoundingBox() const
{
    if(m_node)
        return element(true)->paintBoundingBox();
    return Box();
}

NodeList Element::children() const
{
    if(m_node == nullptr)
        return NodeList();
    NodeList children;
    for(const auto& child : element()->children())
        children.push_back(child.get());
    return children;
}

SVGElement* Element::element(bool layoutIfNeeded) const
{
    auto element = static_cast<SVGElement*>(m_node);
    if(element && layoutIfNeeded)
        element->rootElement()->layoutIfNeeded();
    return element;
}

std::unique_ptr<Document> Document::loadFromFile(const std::string& filename)
{
    std::ifstream fs;
    fs.open(filename);
    if(!fs.is_open())
        return nullptr;
    std::string content;
    std::getline(fs, content, '\0');
    fs.close();
    return loadFromData(content);
}

std::unique_ptr<Document> Document::loadFromData(const std::string& string)
{
    return loadFromData(string.data(), string.size());
}

std::unique_ptr<Document> Document::loadFromData(const char* data)
{
    return loadFromData(data, std::strlen(data));
}

std::unique_ptr<Document> Document::loadFromData(const char* data, size_t length)
{
    std::unique_ptr<Document> document(new Document);
    if(!document->parse(data, length))
        return nullptr;
    return document;
}

std::unique_ptr<Document> Document::loadFromData(const char* data, size_t length, GraphicsCallbacks callbacks)
{
    std::unique_ptr<Document> document(new Document);
    document->callbacks = callbacks;
    if(!document->parse(data, length))
        return nullptr;
    return document;
}

float Document::width() const
{
    return rootElement(true)->intrinsicWidth();
}

float Document::height() const
{
    return rootElement(true)->intrinsicHeight();
}

Box Document::boundingBox() const
{
    return rootElement(true)->localTransform().mapRect(rootElement()->paintBoundingBox());
}

void Document::updateLayout()
{
    m_rootElement->layoutIfNeeded();
}

void Document::forceLayout()
{
    m_rootElement->forceLayout();
}

void Document::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(bitmap.isNull())
        return;
    auto canvas = Canvas::create(bitmap);
    SVGRenderState state(nullptr, nullptr, matrix, SVGRenderMode::Painting, canvas);
    rootElement(true)->render(state);
}

Bitmap Document::renderToBitmap(int width, int height, uint32_t backgroundColor) const
{
    auto intrinsicWidth = rootElement(true)->intrinsicWidth();
    auto intrinsicHeight = rootElement()->intrinsicHeight();
    if(intrinsicWidth == 0.f || intrinsicHeight == 0.f)
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(intrinsicWidth));
        height = static_cast<int>(std::ceil(intrinsicHeight));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * intrinsicHeight / intrinsicWidth));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * intrinsicWidth / intrinsicHeight));
    }

    auto xScale = width / intrinsicWidth;
    auto yScale = height / intrinsicHeight;

    Matrix matrix(xScale, 0, 0, yScale, 0, 0);
    Bitmap bitmap(width, height);
    if(backgroundColor) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

Element Document::elementFromPoint(float x, float y) const
{
    return rootElement(true)->elementFromPoint(x, y);
}

Element Document::getElementById(const std::string& id) const
{
    return m_rootElement->getElementById(id);
}

Element Document::documentElement() const
{
    return m_rootElement.get();
}

SVGRootElement* Document::rootElement(bool layoutIfNeeded) const
{
    if(layoutIfNeeded)
        m_rootElement->layoutIfNeeded();
    return m_rootElement.get();
}

Document::Document(Document&&) = default;
Document& Document::operator=(Document&&) = default;

Document::Document() = default;
Document::~Document() = default;

} // namespace lunasvg
