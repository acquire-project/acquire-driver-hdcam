#include "dcam.error.h"

const char*
dcam_error_to_string(DCAMERR error_code)
{
#define CASE(e, msg)                                                           \
    case e:                                                                    \
        return #e " " msg

    switch (error_code) {
        CASE(DCAMERR_BUSY, "API cannot process in busy state.");
        CASE(DCAMERR_NOTREADY, "API requires ready state.");
        CASE(DCAMERR_NOTSTABLE, "API requires stable or unstable state.");
        CASE(DCAMERR_UNSTABLE, "API does not support in unstable state.");
        CASE(DCAMERR_NOTBUSY, "API requires busy state.");
        CASE(DCAMERR_EXCLUDED,
             "some resource is exclusive and already "
             "used");
        CASE(DCAMERR_COOLINGTROUBLE, "something happens near cooler");
        CASE(DCAMERR_NOTRIGGER,
             "no trigger when necessary. Some camera "
             "supports this error.");
        CASE(DCAMERR_TEMPERATURE_TROUBLE, "camera warns its temperature");
        CASE(DCAMERR_TOOFREQUENTTRIGGER,
             "input too frequent trigger. Some camera "
             "supports this error.");
        CASE(DCAMERR_ABORT, "abort process");
        CASE(DCAMERR_TIMEOUT, "timeout");
        CASE(DCAMERR_LOSTFRAME, "frame data is lost");
        CASE(DCAMERR_MISSINGFRAME_TROUBLE,
             "frame is lost but reason is low level "
             "driver's bug");
        CASE(DCAMERR_INVALIDIMAGE, "hpk format data is invalid data");
        CASE(DCAMERR_NORESOURCE, "not enough resource except memory");
        CASE(DCAMERR_NOMEMORY, "not enough memory");
        CASE(DCAMERR_NOMODULE, "no sub module");
        CASE(DCAMERR_NODRIVER, "no driver");
        CASE(DCAMERR_NOCAMERA, "no camera");
        CASE(DCAMERR_NOGRABBER, "no grabber");
        CASE(DCAMERR_NOCOMBINATION, "no combination on registry");
        CASE(DCAMERR_FAILOPEN, "DEPRECATED");
        CASE(DCAMERR_FRAMEGRABBER_NEEDS_FIRMWAREUPDATE,
             "need to update frame grabber firmware to "
             "use the camera");
        CASE(DCAMERR_INVALIDMODULE, "dcam_init() found invalid module");
        CASE(DCAMERR_INVALIDCOMMPORT, "invalid serial port");
        CASE(DCAMERR_FAILOPENBUS, "the bus or driver are not available");
        CASE(DCAMERR_FAILOPENCAMERA, "camera report error during opening");
        CASE(DCAMERR_DEVICEPROBLEM, "initialization failed");
        CASE(DCAMERR_INVALIDCAMERA, "invalid camera");
        CASE(DCAMERR_INVALIDHANDLE, "invalid camera handle");
        CASE(DCAMERR_INVALIDPARAM, "invalid parameter");
        CASE(DCAMERR_INVALIDVALUE, "invalid property value");
        CASE(DCAMERR_OUTOFRANGE, "value is out of range");
        CASE(DCAMERR_NOTWRITABLE, "the property is not writable");
        CASE(DCAMERR_NOTREADABLE, "the property is not readable");
        CASE(DCAMERR_INVALIDPROPERTYID, "the property id is invalid");
        CASE(DCAMERR_NEWAPIREQUIRED,
             "old API cannot present the value because "
             "only new API need to be "
             "used");
        CASE(DCAMERR_WRONGHANDSHAKE,
             "this error happens DCAM get error code "
             "from camera unexpectedly");
        CASE(DCAMERR_NOPROPERTY,
             "there is no alternative or influence id, "
             "or no more property id");
        CASE(DCAMERR_INVALIDCHANNEL,
             "the property id specifies channel but "
             "channel is invalid");
        CASE(DCAMERR_INVALIDVIEW,
             "the property id specifies channel but "
             "channel is invalid");
        CASE(DCAMERR_INVALIDSUBARRAY,
             "the combination of subarray values are "
             "invalid. e.g. "
             "DCAM_IDPROP_SUBARRAYHPOS + "
             "DCAM_IDPROP_SUBARRAYHSIZE is greater "
             "than the number of horizontal pixel of "
             "sensor.");
        CASE(DCAMERR_ACCESSDENY,
             "the property cannot access during this "
             "DCAM STATUS");
        CASE(DCAMERR_NOVALUETEXT, "the property does not have value text");
        CASE(DCAMERR_WRONGPROPERTYVALUE,
             "at least one property value is wrong");
        CASE(DCAMERR_DISHARMONY,
             "the paired camera does not have same "
             "parameter");
        CASE(DCAMERR_FRAMEBUNDLESHOULDBEOFF,
             "framebundle mode should be OFF under "
             "current property props");
        CASE(DCAMERR_INVALIDFRAMEINDEX, "the frame index is invalid");
        CASE(DCAMERR_INVALIDSESSIONINDEX, "the session index is invalid");
        CASE(DCAMERR_NOCORRECTIONDATA,
             "not take the dark and shading correction "
             "data yet.");
        CASE(DCAMERR_CHANNELDEPENDENTVALUE,
             "each channel has own property value so "
             "can't return overall "
             "property value.");
        CASE(DCAMERR_VIEWDEPENDENTVALUE,
             "each view has own property value so "
             "can't return overall property "
             "value.");
        CASE(DCAMERR_NODEVICEBUFFER,
             "the frame count is larger than device memory size on using "
             "device memory.");
        CASE(DCAMERR_REQUIREDSNAP,
             "the capture mode is sequence on using device memory.");
        CASE(DCAMERR_LESSSYSTEMMEMORY,
             "the system memory size is too small. PC "
             "doesn't have enough "
             "memory or is limited memory by 32bit OS.");
        CASE(DCAMERR_NOTSUPPORT,
             "camera does not support the function or "
             "property with current "
             "props");
        CASE(DCAMERR_FAILREADCAMERA, "failed to read data from camera");
        CASE(DCAMERR_FAILWRITECAMERA, "failed to write data to the camera");
        CASE(DCAMERR_CONFLICTCOMMPORT, "conflict the com port name user set");
        CASE(DCAMERR_OPTICS_UNPLUGGED,
             "Optics part is unplugged so please check "
             "it.");
        CASE(DCAMERR_FAILCALIBRATION, "fail calibration");
        CASE(
          DCAMERR_MISMATCH_CONFIGURATION,
          "mismatch between camera output(connection) and frame grabber specs");
        CASE(DCAMERR_INVALIDMEMBER_3, "3th member variable is invalid value");
        CASE(DCAMERR_INVALIDMEMBER_5, "5th member variable is invalid value");
        CASE(DCAMERR_INVALIDMEMBER_7, "7th member variable is invalid value");
        CASE(DCAMERR_INVALIDMEMBER_8, "7th member variable is invalid value");
        CASE(DCAMERR_INVALIDMEMBER_9, "9th member variable is invalid value");
        CASE(DCAMERR_FAILEDOPENRECFILE, "DCAMREC failed to open the file");
        CASE(DCAMERR_INVALIDRECHANDLE, "DCAMREC is invalid handle");
        CASE(DCAMERR_FAILEDWRITEDATA, "DCAMREC failed to write the data");
        CASE(DCAMERR_FAILEDREADDATA, "DCAMREC failed to read the data");
        CASE(DCAMERR_NOWRECORDING, "DCAMREC is recording data now");
        CASE(DCAMERR_WRITEFULL, "DCAMREC writes full frame of the session");
        CASE(DCAMERR_ALREADYOCCUPIED,
             "DCAMREC handle is already occupied by "
             "other HDCAM");
        CASE(DCAMERR_TOOLARGEUSERDATASIZE,
             "DCAMREC is set the large value to user "
             "data size");
        CASE(DCAMERR_INVALIDWAITHANDLE, "DCAMWAIT is invalid handle");
        CASE(DCAMERR_NEWRUNTIMEREQUIRED,
             "DCAM Module Version is older than the "
             "version that the camera "
             "requests");
        CASE(DCAMERR_VERSIONMISMATCH,
             "Camera returns the error on setting "
             "parameter to limit version");
        CASE(DCAMERR_RUNAS_FACTORYMODE, "Camera is running as a factory mode");
        CASE(DCAMERR_IMAGE_UNKNOWNSIGNATURE,
             "signature of image header is unknown or "
             "corrupted");
        CASE(DCAMERR_IMAGE_NEWRUNTIMEREQUIRED,
             "version of image header is newer than "
             "version that used DCAM "
             "supports");
        CASE(DCAMERR_IMAGE_ERRORSTATUSEXIST,
             "image header stands error status");
        CASE(DCAMERR_IMAGE_HEADERCORRUPTED, "image header value is strange");
        CASE(DCAMERR_IMAGE_BROKENCONTENT, "image content is corrupted");
        CASE(DCAMERR_UNKNOWNMSGID, "unknown message id");
        CASE(DCAMERR_UNKNOWNSTRID, "unknown string id");
        CASE(DCAMERR_UNKNOWNPARAMID, "unkown parameter id");
        CASE(DCAMERR_UNKNOWNBITSTYPE, "unknown bitmap bits type");
        CASE(DCAMERR_UNKNOWNDATATYPE, "unknown frame data type");
        CASE(DCAMERR_NONE, "no error, nothing to have done");
        CASE(DCAMERR_INSTALLATIONINPROGRESS, "installation progress");
        CASE(DCAMERR_UNREACH, "internal error");
        CASE(DCAMERR_UNLOADED, "calling after process terminated");
        CASE(DCAMERR_THRUADAPTER, "");
        CASE(DCAMERR_NOCONNECTION, "HDCAM lost connection to camera");
        CASE(DCAMERR_NOTIMPLEMENT, "not yet implementation");
        CASE(DCAMERR_DELAYEDFRAME,
             "the frame waiting re-load from hardware buffer with SNAPSHOT of "
             "DEVICEBUFFER MODE");
        CASE(DCAMERR_DEVICEINITIALIZING, "device initializing");
        CASE(DCAMERR_APIINIT_INITOPTIONBYTES,
             "DCAMAPI_INIT::initoptionbytes is invalid");
        CASE(DCAMERR_APIINIT_INITOPTION, "DCAMAPI_INIT::initoption is invalid");
        CASE(DCAMERR_INITOPTION_COLLISION_BASE,
             "DCAMERR_INITOPTION_COLLISION_BASE");
        CASE(DCAMERR_INITOPTION_COLLISION_MAX,
             "DCAMERR_INITOPTION_COLLISION_MAX");
        CASE(DCAMERR_MISSPROP_TRIGGERSOURCE,
             "the trigger mode is internal or syncreadout on using device "
             "memory.");
        CASE(DCAMERR_SUCCESS,
             "no error, general success code, app "
             "should check the value is "
             "positive");
#undef CASE
        default:
            if (DCAMERR_INITOPTION_COLLISION_BASE <= error_code ||
                error_code <= DCAMERR_INITOPTION_COLLISION_MAX) {
                return "DCAMERR_INITOPTION_COLLISION_"
                       "BASE(x): there is "
                       "collision with initoption in "
                       "DCAMAPI_INIT.";
            }
    }
    return "DCAMERR: (unknown)";
}
