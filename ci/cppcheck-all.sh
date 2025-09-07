#!/bin/bash

# Configuration begin ---------------------------

SCAN_PATHS=("main")
IGNORE_PATHS=("main/external")
SUPPRESS_FILE=("main/driver/touchDriver/touchDriver.c")

STANDARD="c11"
PLATFORM="esp32-platform.xml"
ENABLE="warning,style,performance,portability"
SUPPRESS=("unusedStructMember" "invalidPrintfArgType_uint")
REPORT_DEST="cppcheck-report"

# Configuration end ---------------------------

CNT_WARNING=0
CNT_STYLE=0
CNT_PERFORMANCE=0
CNT_PORTABILITY=0

SUPPERESS_MERGED=( "${SUPPRESS[@]/#/--suppress=}" )

COMMAND="cppcheck ${SCAN_PATHS[@]} --inline-suppr ${SUPPRESS_FILE[@]} -i${IGNORE_PATHS[@]} --std=$STANDARD --platform=$PLATFORM --enable=$ENABLE ${SUPPERESS_MERGED[@]}"

echo ""
echo "$COMMAND"
CHECK=$( eval "$COMMAND 2>&1" )
DUMMY=$( eval "$COMMAND --xml 2> $REPORT_DEST.xml" )

if ! test -z "$CHECK"; then
    echo "$CHECK"

    mkdir -p $REPORT_DEST/xml
    mv $REPORT_DEST.xml $REPORT_DEST/xml/$REPORT_DEST.xml
    echo ""
    echo "XML report generated"

    WARNINGS=`echo "$CHECK" | grep "warning:" | wc -l`
    if test $(( $WARNINGS > 0 )); then
        CNT_WARNING=$(($CNT_WARNINGS + $WARNINGS))
    fi

    STYLE=`echo "$CHECK" | grep "style:" | wc -l`
    if test $(( $STYLE > 0 )); then
        CNT_STYLE=$(($CNT_STYLE + $STYLE))
    fi

    PERFORMANCE=`echo "$CHECK" | grep "performance:" | wc -l`
    if test $(( $PERFORMANCE > 0 )); then
        CNT_PERFORMANCE=$(($CNT_PERFORMANCE + $PERFORMANCE))
    fi

    PORTABILITY=`echo "$CHECK" | grep "portability:" | wc -l`
    if test $(( $PORTABILITY > 0 )); then
        CNT_PORTABILITY=$(($CNT_PORTABILITY + $PORTABILITY))
    fi

    echo ""
    echo "Generating HTML report using cppcheck-htmlreport"
    RAPORT=`cppcheck-htmlreport --file=$REPORT_DEST/xml/$REPORT_DEST.xml --report-dir=$REPORT_DEST/html --source-dir=.`
    echo ""
    echo "DONE"
fi

echo ""
echo "Summary:"
echo "  Total warnings:           $CNT_WARNING"
echo "  Total style issues:       $CNT_STYLE"
echo "  Total performance issues: $CNT_PERFORMANCE"
echo "  Total portability issues: $CNT_PORTABILITY"
echo ""

if [ "$CNT_WARNING" -ne 0 ] || [ "$CNT_STYLE" -ne 0 ] || [ "$CNT_PERFORMANCE" -ne 0 ] || [ "$CNT_PORTABILITY" -ne 0 ]; then
    exit 1
fi

exit 0