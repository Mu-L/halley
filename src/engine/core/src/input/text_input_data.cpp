#include "halley/core/input/text_input_data.h"

#include "api/clipboard.h"
#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"

using namespace Halley;

TextInputData::TextInputData()
{
}

TextInputData::TextInputData(const String& str)
{
	setText(str);
}

TextInputData::TextInputData(StringUTF32 str)
{
	setText(std::move(str));
}

const StringUTF32& TextInputData::getText() const
{
	return text;
}

void TextInputData::setText(const String& text)
{
	setText(text.getUTF32());
}

void TextInputData::setText(StringUTF32 _text)
{
	if (text != _text) {
		text = std::move(_text);
		const int textSize = int(text.size());
		if (selection.start > textSize) {
			selection.start = textSize;
		}
		if (selection.end > textSize) {
			selection.end = textSize;
		}
		onTextModified();
	}
}

Range<int> TextInputData::getSelection() const
{
	return selection;
}

void TextInputData::setSelection(int sel)
{
	setSelection(Range<int>(sel, sel));
}

void TextInputData::setSelection(Range<int> sel)
{
	const int textSize = int(text.size());
	sel.start = clamp(sel.start, 0, textSize);
	sel.end = clamp(sel.end, 0, textSize);
	if (sel.start > sel.end) {
		std::swap(sel.start, sel.end);
	}
	selection = sel;
}

void TextInputData::setLengthLimits(int min, std::optional<int> max)
{
	minLength = min;
	maxLength = max;
}

int TextInputData::getMinLength() const
{
	return minLength;
}

std::optional<int> TextInputData::getMaxLength() const
{
	return maxLength;
}

void TextInputData::insertText(const String& text)
{
	insertText(text.getUTF32());
}

void TextInputData::insertText(const StringUTF32& t)
{
	if (!t.empty() && !readOnly) {
		size_t insertSize = t.length();
		if (maxLength) {
			const int curSelLen = selection.end - selection.start;
			const size_t finalLen = text.size() - size_t(curSelLen) + insertSize;
			if (int(finalLen) > maxLength.value()) {
				insertSize = size_t(std::max(int64_t(0), int64_t(insertSize) + int64_t(maxLength.value()) - int64_t(finalLen)));
			}
		}

		const auto newEnd = int(selection.start + insertSize);
		setText(text.substr(0, selection.start) + t.substr(0, insertSize) + text.substr(selection.end));
		setSelection(newEnd);
	}
}

void TextInputData::onControlCharacter(TextControlCharacter c, std::shared_ptr<IClipboard> clipboard)
{
	switch (c) {
	case TextControlCharacter::Delete:
		onDelete();
		break;
	case TextControlCharacter::Backspace:
		onBackspace();
		break;
	case TextControlCharacter::Home:
	case TextControlCharacter::PageUp:
		setSelection(0);
		break;
	case TextControlCharacter::End:
	case TextControlCharacter::PageDown:
		setSelection(int(text.size()));
		break;
	case TextControlCharacter::Left:
		setSelection(getSelection().start - 1);
		break;
	case TextControlCharacter::Right:
		setSelection(getSelection().start + 1);
		break;
	default:
		// Ignore other cases
		break;
	}

	if (clipboard) {
		switch (c) {
		case TextControlCharacter::Copy:
			clipboard->setData(String(text));
			break;
		case TextControlCharacter::Paste:
		{
			if (!readOnly) {
				auto str = clipboard->getStringData();
				if (str) {
					insertText(str.value());
				}
			}
			break;
		}
		case TextControlCharacter::Cut:
			if (!readOnly) {
				clipboard->setData(String(text));
				setText(StringUTF32());
			}
			break;
		default:
			break;
		}
	}
}

int TextInputData::getTextRevision() const
{
	return textRevision;
}

Range<int> TextInputData::getTotalRange() const
{
	return Range<int>(0, int(text.size()));
}

void TextInputData::setReadOnly(bool enable)
{
	readOnly = enable;
}

bool TextInputData::isReadOnly() const
{
	return readOnly;
}

void TextInputData::onDelete()
{
	if (readOnly) {
		return;
	}

	if (selection.start == selection.end) {
		if (selection.start < int(text.size())) {
			setText(text.substr(0, selection.start) + text.substr(selection.start + 1));
		}
	} else {
		setText(text.substr(0, selection.start) + text.substr(selection.end));
	}
}

void TextInputData::onBackspace()
{
	if (readOnly) {
		return;
	}

	if (selection.start == selection.end) {
		if (selection.start > 0) { // If selection.s == 0, -1 causes it to overflow (unsigned). Shouldn't do anything in that case.
			const auto start = selection.start;
			setText(text.substr(0, start - 1) + text.substr(start));
			setSelection(start - 1);
		}
	} else {
		setText(text.substr(0, selection.start) + text.substr(selection.end));
	}
}

void TextInputData::onTextModified()
{
	++textRevision;
}
