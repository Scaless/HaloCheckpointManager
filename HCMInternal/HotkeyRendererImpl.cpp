#include "pch.h"
#include "HotkeyRendererImpl.h"
#include "imgui.h"
#include "imgui_internal.h"

void HotkeyRendererImpl::beginHotkeyRebindDialog(std::shared_ptr<RebindableHotkey> hotkey)
{

	PLOG_INFO << "opening new hotkey rebind dialog for " << hotkey->getName();
	mHotkeyToManipulate.reset();
	mHotkeyToManipulate = hotkey; // is this failing?

	if (!hotkey)
	{
		PLOG_ERROR << "what";
	}
	if (!mHotkeyToManipulate)
	{
		PLOG_ERROR << "WHATTT";
	}



	hotkeyDisableRequest = hotkeyDisabler->makeRequest(nameof(HotkeyRendererImpl));
	tempBindingSet = hotkey->getBindings();
	currentSelectedBindingForEditing = 0;

	// since the stack id context is different here (because of HCMInternalGUI BeginWindow call), we need to manually
	// override/calculate the ID so that we can access it from the different context in renderHotkeyRebindDialog
	ImGuiID popup_id = ImHashStr("Rebind Hotkey");
	ImGui::PushOverrideID(popup_id); // since we're calling from a different window context than the OpenPopup
	ImGui::OpenPopup("Rebind Hotkey");
	ImGui::PopID();


}



void HotkeyRendererImpl::renderHotkeyRebindDialog(SimpleMath::Vector2 screenSize)
{

	ImGui::SetNextWindowPos(screenSize / 2, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGuiID popup_id = ImHashStr("Rebind Hotkey");
	ImGui::PushOverrideID(popup_id); // since we're calling from a different window context than the OpenPopup
	if (ImGui::BeginPopupModal("Rebind Hotkey", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (!mHotkeyToManipulate)
		{
			PLOG_ERROR << "ARGHHHHH";
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			ImGui::PopID();
			return;
		}



		for (int i = 0; i < tempBindingSet.size(); i++)
		{
			auto& binding = tempBindingSet.at(i);
			std::string currentBindingText = RebindableHotkey::generateBindingTextSingle(binding);

			bool notBeingEdited = i != currentSelectedBindingForEditing;


			ImGui::BeginChild(std::format("InputField{}", i).c_str(), { 400, 30 }, true, ImGuiWindowFlags_NoResize);
			if (notBeingEdited) ImGui::BeginDisabled();
			ImGui::Text(currentBindingText.c_str());
			if (notBeingEdited) ImGui::EndDisabled();
			ImGui::EndChild();

			// used for hit testing mouse hotkeys
			auto inputFieldPosMin = ImGui::GetItemRectMin();
			auto inputFieldPosMax = ImGui::GetItemRectMax();

			if (notBeingEdited)
			{
				ImGui::SameLine();
				if (ImGui::Button(std::format("Edit###RebindEdit{}", i).c_str()))
				{
					currentSelectedBindingForEditing = i;
				}
			}
			else
			{
				ImGui::SameLine();
				if (ImGui::Button(std::format("Stop###RebindStop{}", i).c_str()))
				{
					currentSelectedBindingForEditing = -1;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button(std::format("Clear###RebindClear{}", i).c_str()))
			{
				binding.clear();
			}

			ImGui::SameLine();
			if (ImGui::Button(std::format("Delete###RebindDelete{}", i).c_str()))
			{
				tempBindingSet.erase(tempBindingSet.begin() + i);
				break; // since we're iterating over the set we're deleting from, best to stop iterating now
			}

			if (!notBeingEdited)
			{
				// now to listen for keypresses
				// code is similiar-ish to HotkeyManager::pollInputs, but like in reverse

				for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1))
				{
					if (ImGui::IsKeyPressed(key))
					{
						// if key is already in the binding set then skip
						bool alreadyExists = false;
						for (auto existingKey : binding)
						{
							if (key == existingKey)
							{
								alreadyExists = true;
								break;
							}
						}
						if (alreadyExists) continue;

						// if the key type is mouse related, we only want to accept it if it's hovered over the input box.
						if (key >= ImGuiKey_Mouse_BEGIN && key < ImGuiKey_Mouse_END)
						{
							if (!ImGui::IsMouseHoveringRect(inputFieldPosMin, inputFieldPosMax, false))
								continue; // ignore this key, check next
						}

						binding.push_back(key);
						break; // only bind one key at a time
					}
				}
			}

		}


		// button to add a new binding
		if (ImGui::Button("Add binding"))
		{
			tempBindingSet.push_back({}); // add empty binding
			currentSelectedBindingForEditing = static_cast<int>(tempBindingSet.size()) - 1; // make it the one selected for editing
		}

		if (ImGui::Button("Save"))
		{
			// remove any empty binding sets
			std::erase_if(tempBindingSet, [](const auto& binding) { return false; });
			


			// literally just this call is fucking crashing. something is FUCKED
			// with tempBindingSet. in some sort of invalid state?
			// is mHotkeyToManipulate valid?
			if (!mHotkeyToManipulate) 
			{ 
				PLOG_ERROR << "what the fuck how the fuck is this happening REEEEEE"; 
			}
			else
			{
				mHotkeyToManipulate->setBindings(tempBindingSet);
			}


			hotkeyDisableRequest = std::nullopt;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();

		if (ImGui::Button("Cancel"))
		{
			hotkeyDisableRequest = std::nullopt;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
	ImGui::PopID();

}



void HotkeyRendererImpl::renderHotkey(std::optional<RebindableHotkeyEnum> hotkeyEnum, int pixelWidth)
{


	if (!hotkeyEnum.has_value())
	{
		ImGui::Dummy({ pixelWidth + 30.f,GUIFrameHeight }); // padding
	}
	else
	{
		if (!hotkeyDefinitions->getAllRebindableHotkeys().contains(hotkeyEnum.value()))
		{
			LOG_ONCE(PLOG_ERROR << "Error: bad hotkey passed to beginHotkeyRebindDialog");
			ImGui::Dummy({ pixelWidth + 30.f,GUIFrameHeight }); // padding
			return;
		}
		auto hotkey = hotkeyDefinitions->getAllRebindableHotkeys().at(hotkeyEnum.value());

		if (!hotkey)
		{
			PLOG_ERROR << "how the hell did this happen fuck";
			ImGui::Dummy({ pixelWidth + 30.f,GUIFrameHeight }); // padding
			return;
		}

		ImGui::PushID(hotkey->getName().data());
		ImGui::BeginChild("HotkeyChild", { (float)pixelWidth, GUIFrameHeight });
		ImGui::AlignTextToFramePadding();
		ImGui::Text(hotkey->getBindingTextShort().data());
		addTooltip(std::format("{} hotkeys: {}", hotkey->getName().data(), hotkey->getBindingText().data()).c_str());
		ImGui::EndChild();

		ImGui::SameLine();

		// button for rebinding hotkey
		if (ImGui::Button("..")) // todo: may need to adjust button size
		{
			ImGui::PopID(); // pop id before calling modal dialog
			beginHotkeyRebindDialog(hotkey);
		}
		else
		{
			ImGui::PopID(); // id must always be popped once either way
		}
		addTooltip(std::format("Rebind hotkeys for {}", hotkey->getName().data()).c_str());


	}
}
