function UpdateMarkings(event)
{
    console.log(event);

    var ChangeType = event.checked;
    var listOfChildren = event.parentElement.parentElement.getElementsByClassName("checkboxClass");
    for(var i = 0; i < listOfChildren.length; i++)
    {
        listOfChildren[i].checked = ChangeType;
    }

    //Update Indeterminates

    var allCheckboxClasses = document.getElementsByClassName("checkboxClass");
        for(var i = allCheckboxClasses.length - 1; i >= 0; i--)
    {
        var allChildrenTypes = allCheckboxClasses[i].parentElement.parentElement.getElementsByClassName("checkboxClass");
var checkCount = 0;
        for(var j = 1; j < allChildrenTypes.length; j++)
            {
                if(allChildrenTypes[j].checked) checkCount++;
            }

            if (allChildrenTypes.length > 1 && (checkCount + 1) == allChildrenTypes.length)
                {
                    allCheckboxClasses[i].checked = true;
                    allCheckboxClasses[i].indeterminate = false;
                }
            else
            if (checkCount > 0 )
                {
                    allCheckboxClasses[i].checked = false;
                    allCheckboxClasses[i].indeterminate = true;
                }
            else 
                allCheckboxClasses[i].indeterminate = false;
    }
    
}

function include(arr,obj) {
    return (arr.indexOf(obj) != -1);
}

function GatherAllTicked()
{
	var checkedList = [];
	var rejectList = [];
	var allCheckboxClasses = document.getElementsByClassName("checkboxClass");
    for(var i = 0; i < allCheckboxClasses.length; i++)
    {
		if (allCheckboxClasses[i].checked == true && include(rejectList, allCheckboxClasses[i].id) == false)
        {
			var allChildrenTypes = allCheckboxClasses[i].parentElement.parentElement.getElementsByClassName("checkboxClass");
			var AllChildrenChecked = true;

			if (allChildrenTypes.length == 1)
				checkedList.push(allCheckboxClasses[i].id);
			else
			{
				for(var j = 1; j < allChildrenTypes.length; j++)
				{
					if (allChildrenTypes[j].checked == false)
						AllChildrenChecked = false;
				}
				if (AllChildrenChecked == true)
				{
					//checkedList.push(allCheckboxClasses[i].id);
					for(var j = 1; j < allChildrenTypes.length; j++) 
					{
						if (allChildrenTypes[j].name == "file")
							checkedList.push(allChildrenTypes[j].id); 
						else 
							rejectList.push(allChildrenTypes[j].id);
					}
				}
			}			
		}
	}
	return checkedList;
}