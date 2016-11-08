var groupsSelect;
var groupsMap = [];
function listener() {
	var groups = JSON.parse(this.responseText);
	var groupName;
	var groupId;
   // Remove the old groups
	for (var i = 0; i < groupsSelect.length; i++) {
		groupsSelect.remove(i);
	}
   // Add the new groups
	for (var i = 0; i < groups.length; i++) {
		groupName = groups[i].name;
		groupId = parseInt(groups[i].id);
		groupsSelect.add(groups[i].name);
		groupsMap[groupName] = groupId;
	}
}
document.addEventListener('DOMContentLoaded', function(event) {
	groupsSelect = document.createElement('select');
	var apiKey = document.getElementById('api_key');
	document.body.appendChild(groupsSelect);
	apiKey.addEventListener('input', function() {
		if (apiKey.value.length === 40) {
			var xhr = new XMLHttpRequest();
			xhr.addEventListener('load', listener);
			xhr.open('GET', 'https://api.groupme.com/v3/groups?token=' + apiKey.value);
			xhr.send();
		}
	});
});
