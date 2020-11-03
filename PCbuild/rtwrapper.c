#include "rtsupport.c"
#include <Windows.h>
#include <Windows.ApplicationModel.h>
#include <Windows.Foundation.h>
#include <Windows.Globalization.h>
#include <Windows.Media.h>
#include <Windows.Networking.h>
#include <Windows.Storage.h>
#include <Windows.System.h>
#include <Windows.UI.h>
#include <Windows.UI.Xaml.h>
#include <Windows.Web.h>
/********* C:\WINDOWS\system32\WinMetadata\Windows.ApplicationModel.winmd ************/
using namespace ABI;
/******* Windows.ApplicationModel.Appointments.IAppointmentManagerStatics 2000005 ********/
/* Windows.Foundation.Metadata.GuidAttribute, [976288257, 23616, 18845, 179, 63, 164, 48, 80, 247, 79, 196] */
/* Version 6030000 */
/* Windows.Foundation.Metadata.ExclusiveToAttribute, ['Windows.ApplicationModel.Appointments.AppointmentManager'] */
static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentManagerStatics_ShowTimeFrameAsync(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentManagerStatics *_this = (Windows::ApplicationModel::Appointments::IAppointmentManagerStatics*)self->_com;
  Windows::Foundation::IAsyncAction *result;
  HRESULT hres;
  Windows::Foundation::DateTime param0;
  Windows::Foundation::TimeSpan param1;
  hres = _this->ShowTimeFrameAsync(param0, param1, &result);
}

static PyMethodDef Windows_ApplicationModel_Appointments_IAppointmentManagerStatics_methods[] = {
  {"ShowTimeFrameAsync", Windows_ApplicationModel_Appointments_IAppointmentManagerStatics_ShowTimeFrameAsync, METH_VARARGS},
  {NULL}
};

static PyType_Slot Windows_ApplicationModel_Appointments_IAppointmentManagerStatics_slots[] = {
  {Py_tp_methods, Windows_ApplicationModel_Appointments_IAppointmentManagerStatics_methods},
  {0, 0}
};

static PyType_Spec Windows_ApplicationModel_Appointments_IAppointmentManagerStatics_spec = {
  "IAppointmentManagerStatics",
  sizeof(RTObject),
  0,
  Py_TPFLAGS_DEFAULT,
  Windows_ApplicationModel_Appointments_IAppointmentManagerStatics_slots,
};

/* rtclass Windows.ApplicationModel.Appointments.Appointment */
/* implements (1, 'Windows.ApplicationModel.Appointments.IAppointment') */
/******* Windows.ApplicationModel.Appointments.IAppointmentParticipant 200000b ********/
/* Windows.Foundation.Metadata.GuidAttribute, [1633560834, 38680, 18043, 131, 251, 178, 147, 161, 145, 33, 222] */
/* Version 6030000 */
static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentParticipant_get_DisplayName(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentParticipant *_this = (Windows::ApplicationModel::Appointments::IAppointmentParticipant*)self->_com;
  HSTRING result;
  HRESULT hres;
  hres = _this->get_DisplayName(&result);
}

static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentParticipant_put_DisplayName(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentParticipant *_this = (Windows::ApplicationModel::Appointments::IAppointmentParticipant*)self->_com;
  HRESULT hres;
  HSTRING param0;
  hres = _this->put_DisplayName(param0);
}

static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentParticipant_get_Address(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentParticipant *_this = (Windows::ApplicationModel::Appointments::IAppointmentParticipant*)self->_com;
  HSTRING result;
  HRESULT hres;
  hres = _this->get_Address(&result);
}

static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentParticipant_put_Address(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentParticipant *_this = (Windows::ApplicationModel::Appointments::IAppointmentParticipant*)self->_com;
  HRESULT hres;
  HSTRING param0;
  hres = _this->put_Address(param0);
}

static PyMethodDef Windows_ApplicationModel_Appointments_IAppointmentParticipant_methods[] = {
  {"put_Address", Windows_ApplicationModel_Appointments_IAppointmentParticipant_put_Address, METH_VARARGS},
  {"put_DisplayName", Windows_ApplicationModel_Appointments_IAppointmentParticipant_put_DisplayName, METH_VARARGS},
  {"get_DisplayName", Windows_ApplicationModel_Appointments_IAppointmentParticipant_get_DisplayName, METH_VARARGS},
  {"get_Address", Windows_ApplicationModel_Appointments_IAppointmentParticipant_get_Address, METH_VARARGS},
  {NULL}
};

static PyType_Slot Windows_ApplicationModel_Appointments_IAppointmentParticipant_slots[] = {
  {Py_tp_methods, Windows_ApplicationModel_Appointments_IAppointmentParticipant_methods},
  {0, 0}
};

static PyType_Spec Windows_ApplicationModel_Appointments_IAppointmentParticipant_spec = {
  "IAppointmentParticipant",
  sizeof(RTObject),
  0,
  Py_TPFLAGS_DEFAULT,
  Windows_ApplicationModel_Appointments_IAppointmentParticipant_slots,
};

/* rtclass Windows.ApplicationModel.Appointments.AppointmentOrganizer */
/* implements (1, 'Windows.ApplicationModel.Appointments.IAppointmentParticipant') */
/******* Windows.ApplicationModel.Appointments.IAppointmentInvitee 200000d ********/
/* Windows.Foundation.Metadata.GuidAttribute, [331286422, 38978, 18779, 176, 231, 239, 143, 121, 192, 112, 29] */
/* Windows.Foundation.Metadata.ExclusiveToAttribute, ['Windows.ApplicationModel.Appointments.AppointmentInvitee'] */
/* Version 6030000 */
static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentInvitee_get_Role(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentInvitee *_this = (Windows::ApplicationModel::Appointments::IAppointmentInvitee*)self->_com;
  Windows::ApplicationModel::Appointments::AppointmentParticipantRole result;
  HRESULT hres;
  hres = _this->get_Role(&result);
}

static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentInvitee_put_Role(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentInvitee *_this = (Windows::ApplicationModel::Appointments::IAppointmentInvitee*)self->_com;
  HRESULT hres;
  Windows::ApplicationModel::Appointments::AppointmentParticipantRole param0;
  hres = _this->put_Role(param0);
}

static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentInvitee_get_Response(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentInvitee *_this = (Windows::ApplicationModel::Appointments::IAppointmentInvitee*)self->_com;
  Windows::ApplicationModel::Appointments::AppointmentParticipantResponse result;
  HRESULT hres;
  hres = _this->get_Response(&result);
}

static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentInvitee_put_Response(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentInvitee *_this = (Windows::ApplicationModel::Appointments::IAppointmentInvitee*)self->_com;
  HRESULT hres;
  Windows::ApplicationModel::Appointments::AppointmentParticipantResponse param0;
  hres = _this->put_Response(param0);
}

static PyMethodDef Windows_ApplicationModel_Appointments_IAppointmentInvitee_methods[] = {
  {"put_Response", Windows_ApplicationModel_Appointments_IAppointmentInvitee_put_Response, METH_VARARGS},
  {"put_Role", Windows_ApplicationModel_Appointments_IAppointmentInvitee_put_Role, METH_VARARGS},
  {"get_Role", Windows_ApplicationModel_Appointments_IAppointmentInvitee_get_Role, METH_VARARGS},
  {"get_Response", Windows_ApplicationModel_Appointments_IAppointmentInvitee_get_Response, METH_VARARGS},
  {NULL}
};

static PyType_Slot Windows_ApplicationModel_Appointments_IAppointmentInvitee_slots[] = {
  {Py_tp_methods, Windows_ApplicationModel_Appointments_IAppointmentInvitee_methods},
  {0, 0}
};

static PyType_Spec Windows_ApplicationModel_Appointments_IAppointmentInvitee_spec = {
  "IAppointmentInvitee",
  sizeof(RTObject),
  0,
  Py_TPFLAGS_DEFAULT,
  Windows_ApplicationModel_Appointments_IAppointmentInvitee_slots,
};

/* rtclass Windows.ApplicationModel.Appointments.AppointmentInvitee */
/* implements (1, 'Windows.ApplicationModel.Appointments.IAppointmentInvitee') */
/* implements (1, 'Windows.ApplicationModel.Appointments.IAppointmentParticipant') */
/******* Windows.ApplicationModel.Appointments.IAppointmentRecurrence 2000012 ********/
/* Version 6030000 */
/* Windows.Foundation.Metadata.GuidAttribute, [3631955587, 5542, 18555, 185, 89, 12, 54, 30, 96, 233, 84] */
/* Windows.Foundation.Metadata.ExclusiveToAttribute, ['Windows.ApplicationModel.Appointments.AppointmentRecurrence'] */
static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentRecurrence_get_Unit(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentRecurrence *_this = (Windows::ApplicationModel::Appointments::IAppointmentRecurrence*)self->_com;
  Windows::ApplicationModel::Appointments::AppointmentRecurrenceUnit result;
  HRESULT hres;
  hres = _this->get_Unit(&result);
}

static PyObject*
Windows_ApplicationModel_Appointments_IAppointmentRecurrence_put_Unit(PyObject *_self, PyObject *args)
{
  RTObject* self = (RTObject*)_self;
  Windows::ApplicationModel::Appointments::IAppointmentRecurrence *_this = (Windows::ApplicationModel::Appointments::IAppointmentRecurrence*)self->_com;
  HRESULT hres;
  Windows::ApplicationModel::Appointments::AppointmentRecurrenceUnit param0;
  hres = _this->put_Unit(param0);
}

