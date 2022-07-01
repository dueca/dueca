% XMLSAVE
%
% Save the current parameter set for 'MODEL_NAME' to xml configuration file.
% This file can then be loaded in your DUECA module using an XMLLOADer instance.

% Prepare model info
subs = cat(1, 'MODEL_NAME', find_system('MODEL_NAME', 'BlockType', 'SubSystem'));
[xml_sizes,xml_state0,xml_Order,xml_Ts] = MODEL_NAME;
parse_discrete_states = DO_DSTATES;
% Create XML document tree.
docNode = com.mathworks.xml.XMLUtils.createDocument('MODEL_NAME');
docRootNode = docNode.getDocumentElement;

% If requested, create Inputs node
if lower(input('Store initial inputs? [y/N]: ', 's')) == 'y'
  disp('Enter initial input vector.');
  disp('Examples:');
  disp('u(:,1)');
  disp('[u_autopilot(:,1) u_pilot_init u_misc]');
  iinputs = input('init_input = ', 's');
  eval(['current_xml_data = ' iinputs ';']);
  if length(current_xml_data) ~= xml_sizes(4)
    disp('init_input has wrong length. (Should be the number of inputs for model MODEL_NAME)');
  else
    count = 1; count_init = 0;
    currentNode = docNode.createElement('InitialInputs');
    PUT_INPUT
    docRootNode.appendChild(currentNode);
  end
  clear iinputs
end

% If the model has continuous states, create continuous states node
count = 1; count_init = 0;
current_xml_data = xml_state0;
if xml_sizes(1) > 0
  currentNode = docNode.createElement('InitialContinuousStates');
  PUT_CONT_STATES
  docRootNode.appendChild(currentNode);
end

% If the model has discrete states, create discrete states node
count_init = count-1; count = 1;
if xml_sizes(2) > 0 && parse_discrete_states
  currentNode = docNode.createElement('InitialDiscreteStates');
  PUT_DISC_STATES
  docRootNode.appendChild(currentNode);
end

% Create Parameter node
currentNode = docNode.createElement('ModelParameters');
PUT_PARAM
docRootNode.appendChild(currentNode);

% Save the XML document.
file = input('Filename for new xml config file: ', 's');
if isempty(regexpi(file, '.+\.xml'))
  file = [file '.xml'];
end
xmlwrite(file,docNode);

% Clear temporary variables
clear file docNode docRootnode currentNode subs s current_xml_data xml_sizes xml_state0 xml_Order xml_Ts count count_init;