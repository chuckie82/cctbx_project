# LIBTBX_PRE_DISPATCHER_INCLUDE_SH export PHENIX_GUI_ENVIRONMENT=1

from __future__ import division
import wxtbx.xtriage
import wxtbx.app
import sys

if (__name__ == "__main__") :
  from mmtbx.scaling import xtriage
  result = xtriage.run(args=sys.argv[1:])
  app = wxtbx.app.CCTBXApp(0)
  frame = wxtbx.xtriage.XtriageFrame(parent=None,
    title="Xtriage",
    size=(900,600))
  frame.SetResult(result)
  frame.Show()
  app.MainLoop()
