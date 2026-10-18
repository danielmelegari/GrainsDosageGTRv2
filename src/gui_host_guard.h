#pragma once
// The native GUI tests reject host automation for the editor-only module selector.
// Cubase need not accept beginEdit/performEdit for a read-only hidden parameter.
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "parameters.h"
namespace aztec {
class GuiHostGuard final : public Steinberg::Vst::IComponentHandler {
 Steinberg::uint32 refs=1;
 Steinberg::tresult edit(Steinberg::Vst::ParamID id){if(id==kUiTab){++rejectedTabEdits;return Steinberg::kResultFalse;}return Steinberg::kResultOk;}
public:
 int rejectedTabEdits=0;
 Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,void** obj) override {
  if(!obj)return Steinberg::kInvalidArgument;*obj=nullptr;
  if(Steinberg::FUnknownPrivate::iidEqual(iid,Steinberg::FUnknown::iid)||Steinberg::FUnknownPrivate::iidEqual(iid,Steinberg::Vst::IComponentHandler::iid)){*obj=static_cast<Steinberg::Vst::IComponentHandler*>(this);addRef();return Steinberg::kResultOk;}return Steinberg::kNoInterface;
 }
 Steinberg::uint32 PLUGIN_API addRef() override{return ++refs;}
 Steinberg::uint32 PLUGIN_API release() override{return --refs;}
 Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID id) override{return edit(id);}
 Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID id,Steinberg::Vst::ParamValue) override{return edit(id);}
 Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID id) override{return edit(id);}
 Steinberg::tresult PLUGIN_API restartComponent(Steinberg::int32) override{return Steinberg::kResultOk;}
};
}
