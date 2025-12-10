/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2023 Apple Inc. All rights reserved.
 * Copyright (C) 2014-2021 Google Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 * Copyright (C) 2025 Samuel Weinig <sam@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include <WebCore/RenderStyleProperties.h>

namespace WebCore {

class RenderStyle final : public RenderStyleProperties {
public:
    RenderStyle(RenderStyle&&);
    RenderStyle& operator=(RenderStyle&&);

    explicit RenderStyle(CreateDefaultStyleTag);
    RenderStyle(const RenderStyle&, CloneTag);

    RenderStyle replace(RenderStyle&&) WARN_UNUSED_RETURN;

    static RenderStyle& defaultStyleSingleton();

    // MARK: - Initialization

    WEBCORE_EXPORT static RenderStyle create();
    static std::unique_ptr<RenderStyle> createPtr();
    static std::unique_ptr<RenderStyle> createPtrWithRegisteredInitialValues(const Style::CustomPropertyRegistry&);

    static RenderStyle clone(const RenderStyle&);
    static RenderStyle cloneIncludingPseudoElements(const RenderStyle&);
    static std::unique_ptr<RenderStyle> clonePtr(const RenderStyle&);

    static RenderStyle createAnonymousStyleWithDisplay(const RenderStyle& parentStyle, DisplayType);
    static RenderStyle createStyleInheritingFromPseudoStyle(const RenderStyle& pseudoStyle);

    void inheritFrom(const RenderStyle&);
    void inheritIgnoringCustomPropertiesFrom(const RenderStyle&);
    void inheritUnicodeBidiFrom(const RenderStyle&);
    inline void inheritColumnPropertiesFrom(const RenderStyle&);
    void fastPathInheritFrom(const RenderStyle&);
    void copyNonInheritedFrom(const RenderStyle&);
    void copyContentFrom(const RenderStyle&);
    void copyPseudoElementsFrom(const RenderStyle&);
    void copyPseudoElementBitsFrom(const RenderStyle&);

    // MARK: - Comparisons

    bool operator==(const RenderStyle&) const;

    StyleDifference diff(const RenderStyle&, OptionSet<StyleDifferenceContextSensitiveProperty>& changedContextSensitiveProperties) const;
    bool diffRequiresLayerRepaint(const RenderStyle&, bool isComposited) const;
    void conservativelyCollectChangedAnimatableProperties(const RenderStyle&, CSSPropertiesBitSet&) const;

    bool scrollAnchoringSuppressionStyleDidChange(const RenderStyle*) const;
    bool outOfFlowPositionStyleDidChange(const RenderStyle*) const;

    bool inheritedEqual(const RenderStyle&) const;
    bool nonInheritedEqual(const RenderStyle&) const;
    bool fastPathInheritedEqual(const RenderStyle&) const;
    bool nonFastPathInheritedEqual(const RenderStyle&) const;
    bool descendantAffectingNonInheritedPropertiesEqual(const RenderStyle&) const;
    bool borderAndBackgroundEqual(const RenderStyle&) const;
    inline bool containerTypeAndNamesEqual(const RenderStyle&) const;
    inline bool columnSpanEqual(const RenderStyle&) const;
    inline bool scrollPaddingEqual(const RenderStyle&) const;
    inline bool fontCascadeEqual(const RenderStyle&) const;
    bool scrollSnapDataEquivalent(const RenderStyle&) const;
    inline bool borderIsEquivalentForPainting(const RenderStyle&) const;

#if !LOG_DISABLED
    void dumpDifferences(TextStream&, const RenderStyle&) const;
#endif

    // MARK: - Style adjustment utilities

    void setColumnStylesFromPaginationMode(PaginationMode);
    inline void addToTextDecorationLineInEffect(Style::TextDecorationLine);
    inline void containIntrinsicWidthAddAuto();
    inline void containIntrinsicHeightAddAuto();
    inline void setGridAutoFlowDirection(Style::GridAutoFlow::Direction);

    void adjustAnimations();
    void adjustTransitions();
    void adjustBackgroundLayers();
    void adjustMaskLayers();
    void adjustScrollTimelines();
    void adjustViewTimelines();

    inline void resetBorder();
    inline void resetBorderExceptRadius();
    inline void resetBorderTop();
    inline void resetBorderRight();
    inline void resetBorderBottom();
    inline void resetBorderLeft();
    inline void resetBorderImage();
    inline void resetBorderRadius();
    inline void resetBorderTopLeftRadius();
    inline void resetBorderTopRightRadius();
    inline void resetBorderBottomLeftRadius();
    inline void resetBorderBottomRightRadius();
    inline void resetMargin();
    inline void resetPadding();
    inline void resetColumnRule();
    inline void resetPageSize();

    // MARK: - Pseudo element/style

    std::optional<PseudoElementType> pseudoElementType() const;
    const AtomString& pseudoElementNameArgument() const;

    std::optional<Style::PseudoElementIdentifier> pseudoElementIdentifier() const;
    void setPseudoElementIdentifier(std::optional<Style::PseudoElementIdentifier>&&);

    inline bool hasAnyPublicPseudoStyles() const;
    inline bool hasPseudoStyle(PseudoElementType) const;
    inline void setHasPseudoStyles(EnumSet<PseudoElementType>);

    RenderStyle* getCachedPseudoStyle(const Style::PseudoElementIdentifier&) const;
    RenderStyle* addCachedPseudoStyle(std::unique_ptr<RenderStyle>);

    bool hasCachedPseudoStyles() const { return m_cachedPseudoStyles && m_cachedPseudoStyles->styles.size(); }
    const PseudoStyleCache* cachedPseudoStyles() const { return m_cachedPseudoStyles.get(); }

    // MARK: - Custom properties

    inline const Style::CustomPropertyData& inheritedCustomProperties() const;
    inline const Style::CustomPropertyData& nonInheritedCustomProperties() const;
    const Style::CustomProperty* customPropertyValue(const AtomString&) const;
    bool customPropertyValueEqual(const RenderStyle&, const AtomString&) const;

    void deduplicateCustomProperties(const RenderStyle&);
    void setCustomPropertyValue(Ref<const Style::CustomProperty>&&, bool isInherited);
    bool customPropertiesEqual(const RenderStyle&) const;

    // MARK: - Custom paint

    void addCustomPaintWatchProperty(const AtomString&);

    // MARK: - Text autosizing

#if ENABLE(TEXT_AUTOSIZING)
    uint32_t hashForTextAutosizing() const;
    bool equalForTextAutosizing(const RenderStyle&) const;

    bool isIdempotentTextAutosizingCandidate() const;
    bool isIdempotentTextAutosizingCandidate(AutosizeStatus overrideStatus) const;
#endif

    // MARK: - Derived Values

    float outlineSize() const; // used value combining `outline-width` and `outline-offset`

    String altFromContent() const;

    const AtomString& hyphenString() const;

    WEBCORE_EXPORT float computedLineHeight() const;
    float computeLineHeight(const Style::LineHeight&) const;

    LayoutBoxExtent imageOutsets(const Style::BorderImage&) const;
    LayoutBoxExtent imageOutsets(const Style::MaskBorder&) const;
    inline bool hasBorderImageOutsets() const;
    inline LayoutBoxExtent borderImageOutsets() const;
    inline LayoutBoxExtent maskBorderOutsets() const;

    // MARK: - Logical Values

    // Logical Inset
    inline const Style::InsetEdge& logicalLeft() const;
    inline const Style::InsetEdge& logicalRight() const;
    inline const Style::InsetEdge& logicalTop() const;
    inline const Style::InsetEdge& logicalBottom() const;

    // Logical Sizing
    inline const Style::PreferredSize& logicalWidth(const WritingMode) const;
    inline const Style::PreferredSize& logicalHeight(const WritingMode) const;
    inline const Style::MinimumSize& logicalMinWidth(const WritingMode) const;
    inline const Style::MinimumSize& logicalMinHeight(const WritingMode) const;
    inline const Style::MaximumSize& logicalMaxWidth(const WritingMode) const;
    inline const Style::MaximumSize& logicalMaxHeight(const WritingMode) const;
    inline const Style::PreferredSize& logicalWidth() const;
    inline const Style::PreferredSize& logicalHeight() const;
    inline const Style::MinimumSize& logicalMinWidth() const;
    inline const Style::MinimumSize& logicalMinHeight() const;
    inline const Style::MaximumSize& logicalMaxWidth() const;
    inline const Style::MaximumSize& logicalMaxHeight() const;
    inline void setLogicalWidth(Style::PreferredSize&&);
    inline void setLogicalHeight(Style::PreferredSize&&);
    inline void setLogicalMinWidth(Style::MinimumSize&&);
    inline void setLogicalMinHeight(Style::MinimumSize&&);
    inline void setLogicalMaxWidth(Style::MaximumSize&&);
    inline void setLogicalMaxHeight(Style::MaximumSize&&);

    // Logical Border
    const BorderValue& borderBefore(const WritingMode) const;
    const BorderValue& borderAfter(const WritingMode) const;
    const BorderValue& borderStart(const WritingMode) const;
    const BorderValue& borderEnd(const WritingMode) const;
    inline const BorderValue& borderBefore() const;
    inline const BorderValue& borderAfter() const;
    inline const BorderValue& borderStart() const;
    inline const BorderValue& borderEnd() const;
    Style::LineWidth borderBeforeWidth(const WritingMode) const;
    Style::LineWidth borderAfterWidth(const WritingMode) const;
    Style::LineWidth borderStartWidth(const WritingMode) const;
    Style::LineWidth borderEndWidth(const WritingMode) const;
    inline Style::LineWidth borderBeforeWidth() const;
    inline Style::LineWidth borderAfterWidth() const;
    inline Style::LineWidth borderStartWidth() const;
    inline Style::LineWidth borderEndWidth() const;

    // Logical Margin
    inline const Style::MarginEdge& marginStart(const WritingMode) const;
    inline const Style::MarginEdge& marginEnd(const WritingMode) const;
    inline const Style::MarginEdge& marginBefore(const WritingMode) const;
    inline const Style::MarginEdge& marginAfter(const WritingMode) const;
    inline const Style::MarginEdge& marginBefore() const;
    inline const Style::MarginEdge& marginAfter() const;
    inline const Style::MarginEdge& marginStart() const;
    inline const Style::MarginEdge& marginEnd() const;
    void setMarginStart(Style::MarginEdge&&);
    void setMarginEnd(Style::MarginEdge&&);
    void setMarginBefore(Style::MarginEdge&&);
    void setMarginAfter(Style::MarginEdge&&);

    // Logical Padding
    inline const Style::PaddingEdge& paddingBefore(const WritingMode) const;
    inline const Style::PaddingEdge& paddingAfter(const WritingMode) const;
    inline const Style::PaddingEdge& paddingStart(const WritingMode) const;
    inline const Style::PaddingEdge& paddingEnd(const WritingMode) const;
    inline const Style::PaddingEdge& paddingBefore() const;
    inline const Style::PaddingEdge& paddingAfter() const;
    inline const Style::PaddingEdge& paddingStart() const;
    inline const Style::PaddingEdge& paddingEnd() const;
    void setPaddingStart(Style::PaddingEdge&&);
    void setPaddingEnd(Style::PaddingEdge&&);
    void setPaddingBefore(Style::PaddingEdge&&);
    void setPaddingAfter(Style::PaddingEdge&&);

    // Logical Aspect Ratio
    inline Style::Number<CSS::Nonnegative> aspectRatioLogicalWidth() const;
    inline Style::Number<CSS::Nonnegative> aspectRatioLogicalHeight() const;
    inline double logicalAspectRatio() const;
    inline BoxSizing boxSizingForAspectRatio() const;

    // Logical ContainIntrinsicSize
    inline const Style::ContainIntrinsicSize& containIntrinsicLogicalWidth() const;
    inline const Style::ContainIntrinsicSize& containIntrinsicLogicalHeight() const;

    // Logical Grid
    inline const Style::GridTrackSizes& gridAutoList(Style::GridTrackSizingDirection) const;
    inline const Style::GridTemplateList& gridTemplateList(Style::GridTrackSizingDirection) const;
    inline const Style::GridPosition& gridItemStart(Style::GridTrackSizingDirection) const;
    inline const Style::GridPosition& gridItemEnd(Style::GridTrackSizingDirection) const;
    inline const Style::GapGutter& gap(Style::GridTrackSizingDirection) const;

    // MARK: - Used Values

    inline float usedLetterSpacing() const;
    inline float usedWordSpacing() const;

    inline PointerEvents usedPointerEvents() const;

    inline StyleAppearance usedAppearance() const;
    inline void setUsedAppearance(StyleAppearance);

    inline Visibility usedVisibility() const;

    inline UserModify usedUserModify() const;
    WEBCORE_EXPORT UserSelect usedUserSelect() const;

    inline Style::Contain usedContain() const;

    // usedContentVisibility will return ContentVisibility::Hidden in a content-visibility: hidden subtree (overriding
    // content-visibility: auto at all times), ContentVisibility::Auto in a content-visibility: auto subtree (when the
    // content is not user relevant and thus skipped), and ContentVisibility::Visible otherwise.
    inline ContentVisibility usedContentVisibility() const;
    inline void setUsedContentVisibility(ContentVisibility);

    inline TransformStyle3D usedTransformStyle3D() const;
    inline float usedPerspective() const;

    // 'touch-action' behavior depends on values in ancestors. We use an additional inherited property to implement that.
    inline Style::TouchAction usedTouchAction() const;
    inline void setUsedTouchAction(Style::TouchAction);

    Color usedScrollbarThumbColor() const;
    Color usedScrollbarTrackColor() const;

    static UsedFloat usedFloat(const RenderElement&); // Returns logical left/right (block-relative).
    static UsedClear usedClear(const RenderElement&); // Returns logical left/right (block-relative).

    inline Style::ZIndex usedZIndex() const;
    inline void setUsedZIndex(Style::ZIndex);

    float computedStrokeWidth(const IntSize& viewportSize) const;
    Color computedStrokeColor() const;
    inline CSSPropertyID usedStrokeColorProperty() const;

#if HAVE(CORE_MATERIAL)
    inline AppleVisualEffect usedAppleVisualEffectForSubtree() const;
    inline void setUsedAppleVisualEffectForSubtree(AppleVisualEffect);
#endif

    // MARK: - has*()

    inline bool hasAnimations() const;
    inline bool hasAnimationsOrTransitions() const;
    inline bool hasAppearance() const;
    inline bool hasAppleColorFilter() const;
    inline bool hasAspectRatio() const;
    inline bool hasAutoLengthContainIntrinsicSize() const;
    inline bool hasBackdropFilter() const;
    inline bool hasBackground() const;
    inline bool hasBackgroundImage() const;
    inline bool hasBlendMode() const;
    inline bool hasBorder() const;
    inline bool hasBorderImage() const;
    inline bool hasBorderRadius() const;
    inline bool hasBoxReflect() const;
    inline bool hasBoxShadow() const;
    inline bool hasClip() const;
    inline bool hasClipPath() const;
    inline bool hasContent() const;
    inline bool hasFill() const;
    inline bool hasFilter() const;
    inline bool hasInlineColumnAxis() const;
    inline bool hasIsolation() const;
    inline bool hasMarkers() const;
    inline bool hasMask() const;
    inline bool hasOffsetPath() const;
    inline bool hasOpacity() const;
    inline bool hasOutline() const;
    inline bool hasOutlineInVisualOverflow() const;
    inline bool hasPerspective() const;
    inline bool hasPositionedMask() const;
    inline bool hasRotate() const;
    inline bool hasScale() const;
    inline bool hasScrollTimelines() const;
    inline bool hasSnapPosition() const;
    inline bool hasStroke() const;
    inline bool hasTextCombine() const;
    inline bool hasTextShadow() const;
    inline bool hasTransform() const;
    inline bool hasTransitions() const;
    inline bool hasTranslate() const;
    inline bool hasUsedAppearance() const;
    inline bool hasUsedContentNone() const;
    inline bool hasViewTimelines() const;
    inline bool hasVisibleBorder() const;
    inline bool hasVisibleBorderDecoration() const;
    inline bool hasExplicitlySetBorderRadius() const;
    inline bool hasExplicitlySetPadding() const;
    bool hasPositiveStrokeWidth() const;
#if HAVE(CORE_MATERIAL)
    inline bool hasAppleVisualEffect() const;
    inline bool hasAppleVisualEffectRequiringBackdropFilter() const;
#endif

    // Whether or not a positioned element requires normal flow x/y to be computed to determine its position.
    inline bool hasStaticInlinePosition(bool horizontal) const;
    inline bool hasStaticBlockPosition(bool horizontal) const;
    inline bool hasOutOfFlowPosition() const;
    inline bool hasInFlowPosition() const;
    inline bool hasViewportConstrainedPosition() const;

    // MARK: - Other predicates

    inline bool isColumnFlexDirection() const;
    inline bool isFixedTableLayout() const;
    inline bool isFloating() const;
    inline bool isInterCharacterRubyPosition() const;
    inline bool isOverflowVisible() const;
    inline bool isReverseFlexDirection() const;
    inline bool isRowFlexDirection() const;
    inline bool isSkippedRootOrSkippedContent() const;
    constexpr bool isDisplayInlineType() const;
    constexpr bool isOriginalDisplayInlineType() const;
    constexpr bool isDisplayFlexibleOrGridFormattingContextBox() const;
    constexpr bool isDisplayDeprecatedFlexibleBox() const;
    constexpr bool isDisplayFlexibleBoxIncludingDeprecatedOrGridFormattingContextBox() const;
    constexpr bool isDisplayRegionType() const;
    constexpr bool isDisplayBlockLevel() const;
    constexpr bool isOriginalDisplayBlockType() const;
    constexpr bool isDisplayTableOrTablePart() const;
    constexpr bool isInternalTableBox() const;
    constexpr bool isRubyContainerOrInternalRubyBox() const;
    constexpr bool isOriginalDisplayListItemType() const;

    constexpr bool doesDisplayGenerateBlockContainer() const;

    inline bool specifiesColumns() const;

    inline bool columnRuleIsTransparent() const;
    inline bool borderLeftIsTransparent() const;
    inline bool borderRightIsTransparent() const;
    inline bool borderTopIsTransparent() const;
    inline bool borderBottomIsTransparent() const;

    inline bool usesStandardScrollbarStyle() const;
    inline bool usesLegacyScrollbarStyle() const;
    bool shouldPlaceVerticalScrollbarOnLeft() const;

    inline bool autoWrap() const;
    inline bool preserveNewline() const;
    inline bool collapseWhiteSpace() const;
    inline bool isCollapsibleWhiteSpace(char16_t) const;
    inline bool breakOnlyAfterWhiteSpace() const;
    inline bool breakWords() const;
    static constexpr bool preserveNewline(WhiteSpaceCollapse);
    static constexpr bool collapseWhiteSpace(WhiteSpaceCollapse);

    // MARK: - Transforms

    // Return true if any transform related property (currently transform, translate, scale, rotate, transformStyle3D or perspective)
    // indicates that we are transforming. The usedTransformStyle3D is not used here because in many cases (such as for deciding
    // whether or not to establish a containing block), the computed value is what matters.
    inline bool hasTransformRelatedProperty() const;
    inline bool preserves3D() const;
    inline bool affectsTransform() const;

    enum class TransformOperationOption : uint8_t {
        TransformOrigin = 1 << 0,
        Translate       = 1 << 1,
        Rotate          = 1 << 2,
        Scale           = 1 << 3,
        Offset          = 1 << 4
    };

    static constexpr OptionSet<TransformOperationOption> allTransformOperations();
    static constexpr OptionSet<TransformOperationOption> individualTransformOperations();

    bool affectedByTransformOrigin() const;

    FloatPoint computePerspectiveOrigin(const FloatRect& boundingBox) const;
    void applyPerspective(TransformationMatrix&, const FloatPoint& originTranslate) const;

    FloatPoint3D computeTransformOrigin(const FloatRect& boundingBox) const;
    void applyTransformOrigin(TransformationMatrix&, const FloatPoint3D& originTranslate) const;
    void unapplyTransformOrigin(TransformationMatrix&, const FloatPoint3D& originTranslate) const;

    // applyTransform calls applyTransformOrigin(), then applyCSSTransform(), followed by unapplyTransformOrigin().
    void applyTransform(TransformationMatrix&, const TransformOperationData& boundingBox) const;
    void applyTransform(TransformationMatrix&, const TransformOperationData& boundingBox, OptionSet<TransformOperationOption>) const;
    void applyCSSTransform(TransformationMatrix&, const TransformOperationData& boundingBox) const;
    void applyCSSTransform(TransformationMatrix&, const TransformOperationData& boundingBox, OptionSet<TransformOperationOption>) const;
    void setPageScaleTransform(float);

    // MARK: - Colors

    Color colorResolvingCurrentColor(CSSPropertyID colorProperty, bool visitedLink) const;

    // Resolves the currentColor keyword, but must not be used for the "color" property which has a different semantic.
    WEBCORE_EXPORT Color colorResolvingCurrentColor(const Style::Color&, bool visitedLink = false) const;

    WEBCORE_EXPORT Color visitedDependentColor(CSSPropertyID, OptionSet<PaintBehavior> = { }) const;
    WEBCORE_EXPORT Color visitedDependentColorWithColorFilter(CSSPropertyID, OptionSet<PaintBehavior> = { }) const;

    WEBCORE_EXPORT Color colorByApplyingColorFilter(const Color&) const;
    WEBCORE_EXPORT Color colorWithColorFilter(const Style::Color&) const;

    Color usedAccentColor(OptionSet<StyleColorOptions>) const;

    // MARK: - Non-property initial values.

    static inline Style::PerspectiveOrigin initialPerspectiveOrigin();
    static inline Style::TransformOrigin initialTransformOrigin();
    static inline Style::Animations initialAnimations();
    static inline Style::Transitions initialTransitions();
    static inline Style::BackgroundLayers initialBackgroundLayers();
    static inline Style::MaskLayers initialMaskLayers();
    static inline Style::BorderImage initialBorderImage();
    static inline Style::MaskBorder initialMaskBorder();
    static inline Style::PageSize initialPageSize();
    static constexpr Style::ZIndex initialUsedZIndex();
#if ENABLE(TEXT_AUTOSIZING)
    static inline Style::LineHeight initialSpecifiedLineHeight();
#endif

private:
    // This constructor is used to implement the replace operation.
    RenderStyle(RenderStyle&, RenderStyle&&);

    const Style::Color& unresolvedColorForProperty(CSSPropertyID, bool visitedLink = false) const;

    inline bool hasAutoLeftAndRight() const;
    inline bool hasAutoTopAndBottom() const;

    static constexpr bool isDisplayInlineType(DisplayType);
    static constexpr bool isDisplayBlockType(DisplayType);
    static constexpr bool isDisplayFlexibleBox(DisplayType);
    static constexpr bool isDisplayGridFormattingContextBox(DisplayType);
    static constexpr bool isDisplayGridBox(DisplayType);
    static constexpr bool isDisplayGridLanesBox(DisplayType);
    static constexpr bool isDisplayFlexibleOrGridFormattingContextBox(DisplayType);
    static constexpr bool isDisplayDeprecatedFlexibleBox(DisplayType);
    static constexpr bool isDisplayListItemType(DisplayType);
    static constexpr bool isDisplayTableOrTablePart(DisplayType);
    static constexpr bool isInternalTableBox(DisplayType);
    static constexpr bool isRubyContainerOrInternalRubyBox(DisplayType);

    bool changeAffectsVisualOverflow(const RenderStyle&) const;
    bool changeRequiresLayout(const RenderStyle&, OptionSet<StyleDifferenceContextSensitiveProperty>& changedContextSensitiveProperties) const;
    bool changeRequiresOutOfFlowMovementLayoutOnly(const RenderStyle&, OptionSet<StyleDifferenceContextSensitiveProperty>& changedContextSensitiveProperties) const;
    bool changeRequiresLayerRepaint(const RenderStyle&, OptionSet<StyleDifferenceContextSensitiveProperty>& changedContextSensitiveProperties) const;
    bool changeRequiresRepaint(const RenderStyle&, OptionSet<StyleDifferenceContextSensitiveProperty>& changedContextSensitiveProperties) const;
    bool changeRequiresRepaintIfText(const RenderStyle&, OptionSet<StyleDifferenceContextSensitiveProperty>& changedContextSensitiveProperties) const;
    bool changeRequiresRecompositeLayer(const RenderStyle&, OptionSet<StyleDifferenceContextSensitiveProperty>& changedContextSensitiveProperties) const;
};

// Map from computed style values (which take zoom into account) to web-exposed values, which are zoom-independent.
inline int adjustForAbsoluteZoom(int, const RenderStyle&);
inline float adjustFloatForAbsoluteZoom(float, const RenderStyle&);
inline LayoutUnit adjustLayoutUnitForAbsoluteZoom(LayoutUnit, const RenderStyle&);
inline LayoutSize adjustLayoutSizeForAbsoluteZoom(LayoutSize, const RenderStyle&);

// Map from zoom-independent style values to computed style values (which take zoom into account).
inline float applyZoom(float, const RenderStyle&);

constexpr BorderStyle collapsedBorderStyle(BorderStyle);

inline bool pseudoElementRendererIsNeeded(const RenderStyle*);
inline bool generatesBox(const RenderStyle&);
inline bool isNonVisibleOverflow(Overflow);

inline bool isVisibleToHitTesting(const RenderStyle&, const HitTestRequest&);

inline bool shouldApplyLayoutContainment(const RenderStyle&, const Element&);
inline bool shouldApplySizeContainment(const RenderStyle&, const Element&);
inline bool shouldApplyInlineSizeContainment(const RenderStyle&, const Element&);
inline bool shouldApplyStyleContainment(const RenderStyle&, const Element&);
inline bool shouldApplyPaintContainment(const RenderStyle&, const Element&);
inline bool isSkippedContentRoot(const RenderStyle&, const Element&);

} // namespace WebCore
