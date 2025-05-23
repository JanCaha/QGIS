# The following has been generated automatically from src/gui/processing/qgsprocessingalgorithmdialogbase.h
QgsProcessingAlgorithmDialogBase.FormatPlainText = QgsProcessingAlgorithmDialogBase.LogFormat.FormatPlainText
QgsProcessingAlgorithmDialogBase.FormatHtml = QgsProcessingAlgorithmDialogBase.LogFormat.FormatHtml
# monkey patching scoped based enum
QgsProcessingAlgorithmDialogBase.DialogMode.Single.__doc__ = "Single algorithm execution mode"
QgsProcessingAlgorithmDialogBase.DialogMode.Batch.__doc__ = "Batch processing mode"
QgsProcessingAlgorithmDialogBase.DialogMode.__doc__ = """Dialog modes.

.. versionadded:: 3.24

* ``Single``: Single algorithm execution mode
* ``Batch``: Batch processing mode

"""
# --
try:
    QgsProcessingAlgorithmDialogBase.__attribute_docs__ = {'algorithmAboutToRun': 'Emitted when the algorithm is about to run in the specified ``context``.\n\nThis signal can be used to tweak the ``context`` prior to the algorithm\nexecution.\n\n.. versionadded:: 3.38\n', 'algorithmFinished': 'Emitted whenever an algorithm has finished executing in the dialog.\n\n.. versionadded:: 3.14\n'}
    QgsProcessingAlgorithmDialogBase.formatStringForLog = staticmethod(QgsProcessingAlgorithmDialogBase.formatStringForLog)
    QgsProcessingAlgorithmDialogBase.__virtual_methods__ = ['setParameters', 'resetAdditionalGui', 'blockAdditionalControlsWhileRunning', 'isFinalized', 'finished', 'runAlgorithm', 'algExecuted']
    QgsProcessingAlgorithmDialogBase.__overridden_methods__ = ['reject', 'closeEvent']
    QgsProcessingAlgorithmDialogBase.__signal_arguments__ = {'algorithmAboutToRun': ['context: QgsProcessingContext'], 'algorithmFinished': ['successful: bool', 'result: Dict[str, object]']}
    QgsProcessingAlgorithmDialogBase.__group__ = ['processing']
except (NameError, AttributeError):
    pass
