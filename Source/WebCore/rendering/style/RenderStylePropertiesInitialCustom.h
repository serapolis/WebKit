/*
 * Copyright (C) 2025 Samuel Weinig <sam@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <WebCore/GraphicsTypes.h>
#include <WebCore/PositionTryOrder.h>
#include <WebCore/RenderStyleBaseInlines.h>
#include <WebCore/RenderStyleProperties.h>
#include <WebCore/SVGRenderStyle.h>
#include <WebCore/ScrollTypes.h>
#include <WebCore/StyleAppearance.h>
#include <WebCore/StyleAppleColorFilterData.h>
#include <WebCore/StyleBackdropFilterData.h>
#include <WebCore/StyleBackgroundData.h>
#include <WebCore/StyleBoxData.h>
#include <WebCore/StyleDeprecatedFlexibleBoxData.h>
#include <WebCore/StyleFillLayers.h>
#include <WebCore/StyleFilterData.h>
#include <WebCore/StyleFlexibleBoxData.h>
#include <WebCore/StyleFontData.h>
#include <WebCore/StyleFontFamily.h>
#include <WebCore/StyleFontFeatureSettings.h>
#include <WebCore/StyleFontPalette.h>
#include <WebCore/StyleFontSizeAdjust.h>
#include <WebCore/StyleFontStyle.h>
#include <WebCore/StyleFontVariantAlternates.h>
#include <WebCore/StyleFontVariantEastAsian.h>
#include <WebCore/StyleFontVariantLigatures.h>
#include <WebCore/StyleFontVariantNumeric.h>
#include <WebCore/StyleFontVariationSettings.h>
#include <WebCore/StyleFontWeight.h>
#include <WebCore/StyleFontWidth.h>
#include <WebCore/StyleGridData.h>
#include <WebCore/StyleGridItemData.h>
#include <WebCore/StyleGridTrackSizingDirection.h>
#include <WebCore/StyleInheritedData.h>
#include <WebCore/StyleMarqueeData.h>
#include <WebCore/StyleMiscNonInheritedData.h>
#include <WebCore/StyleMultiColData.h>
#include <WebCore/StyleNonInheritedData.h>
#include <WebCore/StyleRareInheritedData.h>
#include <WebCore/StyleRareNonInheritedData.h>
#include <WebCore/StyleSurroundData.h>
#include <WebCore/StyleTextAlign.h>
#include <WebCore/StyleTextAutospace.h>
#include <WebCore/StyleTextDecorationLine.h>
#include <WebCore/StyleTextSpacingTrim.h>
#include <WebCore/StyleTextTransform.h>
#include <WebCore/StyleTransformData.h>
#include <WebCore/StyleVisitedLinkColorData.h>
#include <WebCore/StyleWebKitLocale.h>
#include <WebCore/UnicodeBidi.h>
#include <WebCore/ViewTimeline.h>
#include <WebCore/WebAnimationTypes.h>

#if ENABLE(APPLE_PAY)
#include <WebCore/ApplePayButtonPart.h>
#endif

#if HAVE(CORE_MATERIAL)
#include <WebCore/AppleVisualEffect.h>
#endif

namespace WebCore {

constexpr Style::MathDepth RenderStyleProperties::initialMathDepth()
{
    using namespace CSS::Literals;
    return 0_css_integer;
}

constexpr Style::Order RenderStyleProperties::initialOrder()
{
    using namespace CSS::Literals;
    return 0_css_integer;
}

constexpr Style::WebkitBoxFlexGroup RenderStyleProperties::initialBoxFlexGroup()
{
    using namespace CSS::Literals;
    return 1_css_integer;
}

constexpr Style::WebkitBoxOrdinalGroup RenderStyleProperties::initialBoxOrdinalGroup()
{
    using namespace CSS::Literals;
    return 1_css_integer;
}

constexpr Style::SVGGlyphOrientationHorizontal RenderStyleProperties::initialGlyphOrientationHorizontal()
{
    return Style::SVGGlyphOrientationHorizontal::Degrees0;
}

constexpr TextDirection RenderStyleProperties::initialDirection()
{
    return TextDirection::LTR;
}

#if ENABLE(VARIATION_FONTS)

constexpr FontOpticalSizing RenderStyleProperties::initialFontOpticalSizing()
{
    return FontOpticalSizing::Enabled;
}

#endif

constexpr TextRenderingMode RenderStyleProperties::initialTextRendering()
{
    return TextRenderingMode::AutoTextRendering;
}

constexpr FontSmoothingMode RenderStyleProperties::initialFontSmoothing()
{
    return FontSmoothingMode::AutoSmoothing;
}

constexpr WindRule RenderStyleProperties::initialClipRule()
{
    return WindRule::NonZero;
}

constexpr WindRule RenderStyleProperties::initialFillRule()
{
    return WindRule::NonZero;
}

constexpr FlexWrap RenderStyleProperties::initialFlexWrap()
{
    return FlexWrap::NoWrap;
}

inline Color RenderStyleProperties::initialColor()
{
    return Color::black;
}

constexpr Style::LineWidth RenderStyleProperties::initialBorderBottomWidth()
{
    return Style::LineWidth { 3.0f };
}

constexpr Style::LineWidth RenderStyleProperties::initialBorderLeftWidth()
{
    return Style::LineWidth { 3.0f };
}

constexpr Style::LineWidth RenderStyleProperties::initialBorderRightWidth()
{
    return Style::LineWidth { 3.0f };
}

constexpr Style::LineWidth RenderStyleProperties::initialBorderTopWidth()
{
    return Style::LineWidth { 3.0f };
}

constexpr Style::LineWidth RenderStyleProperties::initialColumnRuleWidth()
{
    return Style::LineWidth { 3.0f };
}

constexpr Style::LineWidth RenderStyleProperties::initialOutlineWidth()
{
    return Style::LineWidth { 3.0f };
}

inline Style::BorderImageOutset RenderStyleProperties::initialBorderImageOutset()
{
    return { .values = { Style::BorderImageOutsetValue::Number { 0 } } };
}

inline Style::BorderImageRepeat RenderStyleProperties::initialBorderImageRepeat()
{
    return { .values { NinePieceImageRule::Stretch, NinePieceImageRule::Stretch } };
}

inline Style::BorderImageSlice RenderStyleProperties::initialBorderImageSlice()
{
    return { .values = { Style::BorderImageSliceValue::Percentage { 100 } }, .fill = { std::nullopt } };
}

inline Style::BorderImageWidth RenderStyleProperties::initialBorderImageWidth()
{
    return { .values = { Style::BorderImageWidthValue::Number { 1 } }, .legacyWebkitBorderImage = false };
}

inline Style::MaskBorderOutset RenderStyleProperties::initialMaskBorderOutset()
{
    return { .values = { Style::MaskBorderOutsetValue::Number { 0 } } };
}

inline Style::MaskBorderRepeat RenderStyleProperties::initialMaskBorderRepeat()
{
    return { .values { NinePieceImageRule::Stretch, NinePieceImageRule::Stretch } };
}

inline Style::MaskBorderSlice RenderStyleProperties::initialMaskBorderSlice()
{
    return { .values = { Style::MaskBorderSliceValue::Number { 0 } }, .fill = { std::nullopt } };
}

inline Style::MaskBorderWidth RenderStyleProperties::initialMaskBorderWidth()
{
    return { .values = { CSS::Keyword::Auto { } } };
}

inline Style::SVGPaint RenderStyleProperties::initialFill()
{
    return Style::Color { CSS::Keyword::Black { } };
}

constexpr Style::WebkitLineBoxContain RenderStyleProperties::initialLineBoxContain()
{
    return { Style::WebkitLineBoxContainValue::Block, Style::WebkitLineBoxContainValue::Inline, Style::WebkitLineBoxContainValue::Replaced };
}

constexpr Style::TextEmphasisPosition RenderStyleProperties::initialTextEmphasisPosition()
{
    return { Style::TextEmphasisPositionValue::Over, Style::TextEmphasisPositionValue::Right };
}

constexpr Style::PositionVisibility RenderStyleProperties::initialPositionVisibility()
{
    return Style::PositionVisibilityValue::AnchorsVisible;
}

} // namespace WebCore
